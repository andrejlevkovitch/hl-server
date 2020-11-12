// Server.hpp

#pragma once

#include "ss/AbstractRequestHandler.hpp"
#include <boost/asio/io_context.hpp>
#include <memory>

namespace ss {
namespace asio = boost::asio;

class ServerBuilder;
class ServerImpl;

class Server {
  friend ServerBuilder;

public:
  enum Protocol { Tcp, Unix };

  Server &asyncRun();

  Server &stop();

private:
  Server() = default;

private:
  std::shared_ptr<ServerImpl> impl_;
};

using ServerPtr = std::shared_ptr<Server>;

class ServerBuilder {
public:
  ServerBuilder(asio::io_context &context);

  ServerBuilder &setRequestHandlerFactory(RequestHandlerFactory factory);

  /**\param endpoint must contains protocol, ip and port
   */
  ServerBuilder &setEndpoint(Server::Protocol protocol,
                             std::string_view endpoint);

  ServerPtr build() const noexcept(false);

private:
  asio::io_context &ioContext_;

  std::shared_ptr<AbstractRequestHandlerFactory> reqHandlerFactory_;
  Server::Protocol                               protocol_;
  std::string                                    endpoint_;
};
} // namespace ss
