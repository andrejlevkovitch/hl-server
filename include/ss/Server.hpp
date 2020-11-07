// Server.hpp

#pragma once

#include <memory>
#include <string_view>

namespace ss {
class ServerImp;

class Server {
public:
  Server() noexcept;
  ~Server() noexcept;

  /**\brief start server for processing
   *
   * \note it does not start io_context object! You must start it manually after
   * run the server object
   */
  void run(std::string_view host, unsigned short port) noexcept;

private:
  std::shared_ptr<ServerImp> imp_;
};
} // namespace ss
