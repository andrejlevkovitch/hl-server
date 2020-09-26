// Server.cpp

#include "ss/Server.hpp"
#include "logs.hpp"
#include "ss/Context.hpp"
#include "ss/Session.hpp"
#include "ss/SessionPool.hpp"
#include <boost/asio/coroutine.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/yield.hpp>

namespace ss {
namespace asio   = boost::asio;
using tcp        = asio::ip::tcp;
using error_code = boost::system::error_code;

class ServerImp
    : public std::enable_shared_from_this<ServerImp>
    , public asio::coroutine {
public:
  explicit ServerImp(uint maxSessionCount) noexcept
      : sigSet_{Context::ioContext(), SIGINT, SIGTERM}
      , acceptor_{Context::ioContext()}
      , sessionPool_{}
      , maxSessionCount_{maxSessionCount} {
    // set callbacks for context
    sigSet_.async_wait([this](const error_code &error, int) {
      if (error.failed()) {
        LOG_ERROR(error.message());
      }

      // at first we need stop accepting new connections
      LOG_DEBUG("stop acceptor");
      if (acceptor_.is_open()) {
        this->stopAccepting();
        error_code err;
        acceptor_.close(err);
        if (err.failed()) {
          LOG_ERROR(err.message());
        }
      }

      // at second - close already opened connections
      LOG_DEBUG("close all opened sessions");
      this->sessionPool_.closeAllSessions();

      // at last - stop context
      LOG_DEBUG("stop context");
      Context::ioContext().stop();
    });

    // if we stop accepting after filling pool, then we must start accepting
    // again after removing some session from pool
    if (maxSessionCount_ != 0) {
      sessionPool_.atSessionClose.connect([this] {
        if (this->maxSessionCount_ > this->sessionPool_.size()) {
          std::shared_ptr<ServerImp> self = this->shared_from_this();
          this->startAccepting(std::move(self));
        }
      });
    }
  }

  void run(std::shared_ptr<ServerImp> self,
           std::string_view           host,
           unsigned short             port) noexcept {
    error_code                  error;
    tcp::resolver               resolver{Context::ioContext()};
    tcp::resolver::results_type resolvResults =
        resolver.resolve(tcp::v4(), host, std::to_string(port), error);
    if (error.failed()) {
      LOG_FAILURE(error.message());
    }

    tcp protocol = resolvResults->endpoint().protocol();
    acceptor_.open(protocol, error);
    if (error.failed()) {
      LOG_FAILURE(error.message());
    }

    // XXX after closing server with some client connected tcp socket on
    // server side will be set in TIME_WAIT (see `netstat -ap`). This is a
    // normal (for tcp socket), but in this case we can not bind the address.
    // So we need set this option for acceptor
    acceptor_.set_option(tcp::acceptor::reuse_address{true}, error);
    if (error.failed()) {
      LOG_FAILURE(error.message());
    }

    bool binded = false;
    for (auto resolvIter = resolvResults.begin();
         resolvIter != resolvResults.end();
         ++resolvIter) {
      tcp::endpoint endpoint = resolvIter->endpoint();
      acceptor_.bind(resolvIter->endpoint(), error);
      if (error.failed() == false) {
        LOG_INFO("listen endpoint: %1%", endpoint);
        binded = true;
        break;
      } else {
        LOG_ERROR(error.message());
      }
    }

    if (binded == false) {
      LOG_FAILURE("can't bind any endpoint");
    }

    acceptor_.listen(tcp::socket::max_listen_connections, error);
    if (error.failed()) {
      LOG_FAILURE(error.message());
    }

    this->startAccepting(std::move(self));
  }

private:
  void startAccepting(std::shared_ptr<ServerImp> self) noexcept {
    LOG_DEBUG("start accepting");

    // start acceptor asynchronously
    this->doAccept(std::move(self),
                   error_code{},
                   tcp::socket{Context::ioContext()});
  }

  void stopAccepting() noexcept {
    LOG_DEBUG("stop accepting");
    error_code error;

    acceptor_.cancel(error);
    if (error.failed()) {
      LOG_ERROR(error.message());
    }
  }

  void doAccept(std::shared_ptr<ServerImp> self,
                const error_code &         error,
                tcp::socket                sock) noexcept {
    if (!error.failed()) {
      reenter(this) {
        for (;;) {
          yield acceptor_.async_accept(std::bind(&ServerImp::doAccept,
                                                 this,
                                                 std::move(self),
                                                 std::placeholders::_1,
                                                 std::placeholders::_2));

          // create and start new session
          {
            error_code                     err;
            [[maybe_unused]] tcp::endpoint remote_endpoint =
                sock.remote_endpoint(err);
            if (err.failed()) {
              LOG_ERROR(err.message());
              continue;
            }
            LOG_INFO("accept conntection from: %1%", remote_endpoint);

            Session *session =
                sessionPool_.append(std::make_unique<Session>(std::move(sock)));
            LOG_DEBUG("new session added");

            if (session == nullptr) {
              LOG_ERROR("session doesn't append");
              continue;
            }

            session->start();
          }

          // in this case stop accept any new sockets
          if (maxSessionCount_ != 0 &&
              maxSessionCount_ <= sessionPool_.size()) {
            LOG_DEBUG("stop accepting, limit reached: %1%", maxSessionCount_);

            stopAccepting();

            // XXX after pool had filled we have to stop accepting. But, if some
            // session will be removed from pool, we need start it again. So, DO
            // NOT use here break! Because in case `break` we can not restart
            // the coroutine
            yield return;
          }
        }
      }
    } else if (error == asio::error::operation_aborted) {
      LOG_DEBUG("accepting canceled");
    } else {
      LOG_ERROR(error.message());
    }
  }

private:
  asio::signal_set sigSet_;

  tcp::acceptor acceptor_;

  SessionPool sessionPool_;
  size_t      maxSessionCount_;
};

Server::Server(uint maxSessionCount) noexcept
    : imp_{std::make_shared<ServerImp>(maxSessionCount)} {
  LOG_DEBUG("Server constructor");
}

Server::~Server() noexcept {
  LOG_DEBUG("Server destructor");
}

void Server::run(std::string_view ip, unsigned short port) noexcept {
  LOG_DEBUG("Server run");
  imp_->run(imp_, ip, port);
}
} // namespace ss
