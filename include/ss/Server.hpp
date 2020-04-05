// Server.hpp

#pragma once

#include <memory>
#include <string_view>

namespace ss {
class ServerImp;

class Server {
public:
  explicit Server(unsigned int maxSessionCount) noexcept;
  ~Server() noexcept;

  void run(std::string_view host, unsigned int port) noexcept;

private:
  std::shared_ptr<ServerImp> imp_;
};
} // namespace ss