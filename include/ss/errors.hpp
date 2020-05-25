// errors.hpp

#pragma once

#include <boost/system/error_code.hpp>

namespace ss {
using error_code = boost::system::error_code;

namespace error {
enum SessionErrors {
  Success = 0,
  PartialData, // in buffer contains partial request
  Size,
};

class SessionErrorCategory final : public boost::system::error_category {
public:
  const char *name() const noexcept override {
    return "SessionErrorCategory";
  }

  std::string message(int ev) const override {
    SessionErrors condition = static_cast<SessionErrors>(ev);

    switch (condition) {
    case SessionErrors::PartialData:
      return "partial data in request buffer";
    default:
      return "Unkhnown error condition: " + std::to_string(ev);
    }
  }
};

error_code make_error_code(SessionErrors ev);

bool isSessionErrorCategory(const boost::system::error_category &category);
} // namespace error
} // namespace ss

namespace boost::system {
template <>
struct is_error_condition_enum<ss::error::SessionErrors> {
  static const bool valud = true;
};
} // namespace boost::system
