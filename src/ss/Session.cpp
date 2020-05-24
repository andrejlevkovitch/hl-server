// Session.cpp

#include "ss/Session.hpp"
#include "logs.hpp"
#include "ss/Context.hpp"
#include <boost/asio/coroutine.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/yield.hpp>
#include <iterator>
#include <ss/errors.hpp>

#define REQUEST_BUFFER_RESERVED  1024 * 1000 // 1Mb
#define RESPONSE_BUFFER_RESERVED 1024 * 1000

namespace ss {
using Connection = boost::signals2::connection;

class SessionImp : public asio::coroutine {
public:
  SessionImp(tcp::socket sock, CloseSignal &mainClose) noexcept
      : sock_{std::move(sock)} {
    req_.reserve(REQUEST_BUFFER_RESERVED);
    res_.reserve(RESPONSE_BUFFER_RESERVED);

    closeConnection = this->atClose_.connect([&mainClose]() {
      mainClose();
    });
  }

  /**\param self shared ptr to the SessionImp. We start some asynchronous
   * operations for the session, so if session will be close and some
   * asynchronous operation left - it cause error about using heap after free.
   * So the self shared pointer exists all time while asynchronos operations
   * (with the SessionImp) exists
   */
  void start(std::shared_ptr<SessionImp> self) noexcept {
    this->operator()(std::move(self), error_code{}, 0);
  }

  void close() noexcept {
    LOG_DEBUG("try close session");
    if (sock_.is_open()) {
      error_code error;

      sock_.cancel(error);
      if (error.failed()) {
        LOG_ERROR(error.message());
      }

      sock_.shutdown(tcp::socket::shutdown_both, error);
      if (error.failed()) {
        LOG_ERROR(error.message());
      }

      sock_.close(error);
      if (error.failed()) {
        LOG_ERROR(error.message());
      }
    }

    // emit signal about closing session
    atClose_();
    closeConnection.disconnect();
  }

private:
  void operator()(std::shared_ptr<SessionImp> self,
                  const error_code &          error,
                  size_t                      transfered) noexcept {
    if (error.failed() == false) {
      reenter(this) {
        for (;;) {
          yield asio::async_read(sock_,
                                 asio::dynamic_buffer(req_),
                                 asio::transfer_at_least(1),
                                 std::bind(&SessionImp::operator(),
                                           this,
                                           std::move(self),
                                           std::placeholders::_1,
                                           std::placeholders::_2));
          LOG_INFO("readed: %1.3fKb", transfered / 1024.);

          // request handling
          {
            AbstractHandlerFactory *factory = Context::getHandlerFactory();
            if (factory == nullptr) {
              LOG_ERROR("handler factory not set");
              close();
              break;
            }

            Handler handler = factory->getRequestHandler();
            if (handler == nullptr) {
              LOG_ERROR("invalid handler");
              close();
              break;
            }

            LOG_DEBUG("handle request");

            error_code err = handler->handle(req_, res_);
            // error_code err = handler->handle(requestIterator->str(), res_);
            if (err.failed() &&
                err.value() == error::SessionErrors::PartialData) {
              // so we need read more
              LOG_DEBUG("read more");
              continue;
            }

            if (err.failed()) {
              LOG_ERROR(err.message());
              close();
              break;
            }

            if (res_.empty()) {
              LOG_ERROR("response is empty");
              close();
              break;
            }
          }

          yield asio::async_write(sock_,
                                  asio::dynamic_buffer(res_),
                                  asio::transfer_all(),
                                  std::bind(&SessionImp::operator(),
                                            this,
                                            std::move(self),
                                            std::placeholders::_1,
                                            std::placeholders::_2));

          LOG_INFO("writed: %1.3fKb", transfered / 1024.);

          // clear buffers
          req_.clear();
          res_.clear();
        }
      }
    } else if (error == asio::error::eof) {
      LOG_DEBUG("client close socket");
      close();
    } else {
      LOG_WARNING(error.message());
      close();
    }
  }

public:
  Connection closeConnection;

private:
  tcp::socket sock_;

  /**\brief request buffer
   */
  std::string req_;

  /**\brief response buffer
   */
  std::string res_;

  CloseSignal atClose_;
};

Session::Session(tcp::socket sock) noexcept
    : imp_{std::make_shared<SessionImp>(std::move(sock), atClose)} {
  LOG_DEBUG("session opened");
}

Session::~Session() noexcept {
  imp_->closeConnection.disconnect();

  LOG_DEBUG("session closed");
}

void Session::start() noexcept {
  LOG_DEBUG("start session");
  return imp_->start(imp_);
}

void Session::close() noexcept {
  return imp_->close();
}
} // namespace ss
