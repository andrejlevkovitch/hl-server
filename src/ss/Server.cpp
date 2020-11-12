// Server.cpp

#include "ss/Server.hpp"
#include "logs.hpp"
#include <boost/asio/coroutine.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/yield.hpp>
#include <list>
#include <regex>

#define REQ_BUFFER_RESERVED 1024 * 1000
#define RES_BUFFER_RESERVED 1024 * 1000


namespace ss {
namespace asio = boost::asio;

using tcp             = asio::ip::tcp;
using stream_protocol = asio::local::stream_protocol;


template <typename Protocol>
class Session final : public asio::coroutine {
public:
  using Socket = asio::basic_stream_socket<Protocol>;

  Session(Socket socket, RequestHandler handler) noexcept
      : socket_{std::move(socket)}
      , reqHandler_{handler} {
    LOG_TRACE("construct session");

    reqBuffer_.reserve(REQ_BUFFER_RESERVED);
    resBuffer_.reserve(RES_BUFFER_RESERVED);
  }

  void start(std::shared_ptr<Session> self) {
    LOG_TRACE("start new session");

    error_code err = self->reqHandler_->atSessionStart();
    if (err.failed()) {
      LOG_ERROR(err.message());
      LOG_WARNING("session doesn't start, because get handler error");
      return;
    }

    self->operator()(std::move(self), error_code{}, 0);
  }

  void close() {
    if (socket_.is_open() == false) {
      LOG_WARNING("session already closed");
      return;
    }

    LOG_TRACE("close session");


    error_code err = reqHandler_->atSessionClose();
    if (err.failed()) {
      LOG_ERROR(err.message());
    }

    // cancel all async operation with this socket
    socket_.cancel(err);
    if (err.failed()) {
      LOG_ERROR(err.message());
    }

    socket_.shutdown(Socket::shutdown_both, err);
    if (err.failed()) {
      LOG_ERROR(err.message());
    }

    socket_.close(err);
    if (err.failed()) {
      LOG_ERROR(err.message());
    }
  }

  bool isOpen() const {
    return socket_.is_open();
  }


private:
  void
  operator()(std::shared_ptr<Session> self, error_code err, size_t transfered) {
    if (err.failed()) {
      switch (err.value()) {
      case asio::error::eof:
        LOG_DEBUG("client close connection");
        break;
      case asio::error::operation_aborted:
        LOG_DEBUG("session canceled");
        break;
      default:
        LOG_ERROR(err.message());
        break;
      }

      self->close();

      LOG_DEBUG("end of session coroutine");
      return;
    }

    reenter(this) {
      for (;;) {
      ReadMore:
        yield asio::async_read(socket_,
                               asio::dynamic_buffer(reqBuffer_),
                               asio::transfer_at_least(1),
                               std::bind(&Session::operator(),
                                         this,
                                         std::move(self),
                                         std::placeholders::_1,
                                         std::placeholders::_2));

        LOG_DEBUG("readed: %1.3fKb", transfered / 1024.);


        // request handling
        {
          std::string_view reqBufferView = reqBuffer_;
          for (;;) {
            size_t reqIgnoreLength = 0;
            err                    = reqHandler_->handle(reqBufferView,
                                      std::back_inserter(resBuffer_),
                                      reqIgnoreLength);
            if (err.failed() == false) {
              if (reqIgnoreLength == 0 ||
                  reqIgnoreLength >= reqBufferView.size()) {
                reqBuffer_.clear();
                break;
              } else {
                reqBufferView =
                    std::string_view{reqBufferView.data() + reqIgnoreLength,
                                     reqBufferView.size() - reqIgnoreLength};
                continue;
              }
            } else { // if some error caused
              if (error::isSessionErrorCategory(err.category()) &&
                  err.value() == error::SessionError::PartialData) {
                reqBuffer_.erase(0,
                                 reqBufferView.data() - reqBuffer_.data() +
                                     reqIgnoreLength);

                LOG_WARNING("partial data in request: %1.3fKb",
                            reqBuffer_.size());

                break;
              }

              // unexpected errors
              LOG_ERROR(err.message());

              close();
              return;
            }
          }
        }

        if (resBuffer_.empty()) {
          goto ReadMore;
        }

        yield asio::async_write(socket_,
                                asio::dynamic_buffer(resBuffer_),
                                asio::transfer_all(),
                                std::bind(&Session::operator(),
                                          this,
                                          std::move(self),
                                          std::placeholders::_1,
                                          std::placeholders::_2));

        LOG_DEBUG("writed: %1.3fKb", transfered / 1024.);

        // clear after every write
        resBuffer_.clear();
      }
    }
  }

private:
  Socket         socket_;
  RequestHandler reqHandler_;
  std::string    reqBuffer_;
  std::string    resBuffer_;
};


class ServerImpl {
public:
  virtual ~ServerImpl() = default;


  virtual void startAccepting(std::shared_ptr<ServerImpl> self) noexcept = 0;

  virtual asio::io_context &ioContext() noexcept = 0;

  virtual void stopAccepting() noexcept(false) = 0;

  virtual void closeAllSessions() = 0;
};


template <typename Protocol>
class ServerImplStream
    : public ServerImpl
    , public asio::coroutine {
public:
  using Endpoint   = typename Protocol::endpoint;
  using SessionPtr = std::shared_ptr<Session<Protocol>>;
  using Socket     = asio::basic_stream_socket<Protocol>;
  using Acceptor   = asio::basic_socket_acceptor<Protocol>;

  ServerImplStream(asio::io_context &    ioContext,
                   Endpoint              endpoint,
                   RequestHandlerFactory reqHandlerFactory)
      : acceptor_{ioContext}
      , reqHandlerFactory_{std::move(reqHandlerFactory)} {
    LOG_TRACE("construct sever");

    Protocol protocol = endpoint.protocol();
    acceptor_.open(protocol);
    acceptor_.set_option(typename Acceptor::reuse_address(true));
    acceptor_.bind(endpoint);
    acceptor_.listen(Socket::max_listen_connections);
  }

  asio::io_context &ioContext() noexcept override {
    return acceptor_.get_io_context();
  }

  void startAccepting(std::shared_ptr<ServerImpl> self) noexcept override {
    LOG_TRACE("start accepting");

    (*this)(self, error_code{}, Socket{self->ioContext()});
  }

  void stopAccepting() noexcept(false) override {
    LOG_TRACE("stop accepting");

    acceptor_.cancel();
  }

  void closeAllSessions() override {
    LOG_TRACE("close all sessions");

    for (SessionPtr &session : sessions_) {
      if (session->isOpen()) {
        session->close();
      }
    }

    sessions_.clear();
  }


private:
  void operator()(std::shared_ptr<ServerImpl> self,
                  error_code                  err,
                  Socket                      socket) noexcept {
    if (err.failed()) {
      if (err.value() == asio::error::operation_aborted) {
        LOG_DEBUG("accepting canceled");
      } else {
        LOG_ERROR(err.message());
      }

      LOG_DEBUG("break the server coroutine");
      return;
    }

    reenter(this) {
      for (;;) {
        yield acceptor_.async_accept(std::bind(&ServerImplStream::operator(),
                                               this,
                                               std::move(self),
                                               std::placeholders::_1,
                                               std::placeholders::_2));

        LOG_DEBUG("accept new connection");
        try {
          LOG_DEBUG("accept connection from: %1%", socket.remote_endpoint());

          // at first remove already closed sessions
          sessions_.remove_if([](const SessionPtr &session) {
            if (session->isOpen() == false) {
              return true;
            }
            return false;
          });


          RequestHandler reqHandler = reqHandlerFactory_->makeRequestHandler();
          SessionPtr     session =
              std::make_shared<Session<Protocol>>(std::move(socket),
                                                  std::move(reqHandler));


          session->start(session);
          sessions_.emplace_back(std::move(session));

          LOG_DEBUG("sessions opened: %1%", sessions_.size());
        } catch (std::exception &e) {
          LOG_ERROR(e.what());
        }
      }
    }
  }


private:
  Acceptor              acceptor_;
  RequestHandlerFactory reqHandlerFactory_;

  std::list<SessionPtr> sessions_;
};


Server &Server::asyncRun() {
  impl_->startAccepting(impl_);
  return *this;
}

Server &Server::stop() {
  impl_->stopAccepting();
  impl_->closeAllSessions();
  return *this;
}


ServerBuilder::ServerBuilder(asio::io_context &ioContext)
    : ioContext_{ioContext} {
}

ServerBuilder &
ServerBuilder::setRequestHandlerFactory(RequestHandlerFactory factory) {
  reqHandlerFactory_ = std::move(factory);
  return *this;
}

ServerBuilder &ServerBuilder::setEndpoint(Server::Protocol protocol,
                                          std::string_view endpoint) {
  protocol_ = protocol;
  endpoint_ = endpoint;
  return *this;
}

ServerPtr ServerBuilder::build() const {
  if (reqHandlerFactory_ == nullptr) {
    LOG_THROW(std::invalid_argument, "invalid request handler factory");
  }


  std::shared_ptr<ServerImpl> impl;
  switch (protocol_) {
  case Server::Protocol::Tcp: {
    std::regex  hostAndPortReg{R"(([^:]+):(\d{1,5}))"};
    std::smatch match;

    if (std::regex_match(endpoint_, match, hostAndPortReg) == false) {
      LOG_THROW(std::invalid_argument, "invalid host or port");
    }

    std::string host = match[1];
    std::string port = match[2];


    error_code err;
    auto       addr = asio::ip::make_address(host, err);
    if (err.failed()) {
      LOG_THROW(std::invalid_argument, "invalid address: %1%", host);
    }

    tcp::endpoint endpoint(addr, std::stoi(port));

    LOG_DEBUG("listen endpoint: %1%", endpoint);


    impl = std::make_shared<ServerImplStream<tcp>>(ioContext_,
                                                   endpoint,
                                                   reqHandlerFactory_);
  } break;
  case Server::Protocol::Unix: {
    stream_protocol::endpoint endpoint{endpoint_};

    LOG_DEBUG("listen endpoint: %1%", endpoint);


    impl =
        std::make_shared<ServerImplStream<stream_protocol>>(ioContext_,
                                                            endpoint,
                                                            reqHandlerFactory_);
  } break;
  }


  ServerPtr retval = std::make_shared<Server>(Server{});
  retval->impl_    = impl;

  return retval;
}
} // namespace ss
