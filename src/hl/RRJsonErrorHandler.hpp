// RRJsonErrorHandler.hpp

#pragma once

#include <nlohmann/json-schema.hpp>
#include <sstream>

namespace hl {
class RRJsonErrorHandler : public nlohmann::json_schema::basic_error_handler {
public:
  void error(const nlohmann::json::json_pointer &ptr,
             const nlohmann::json &              instance,
             const std::string &                 message) override {
    ss << "Error: " << ptr << " - " << message << std::endl;

    nlohmann::json_schema::basic_error_handler::error(ptr, instance, message);
  }

  std::stringstream ss;
};
} // namespace hl
