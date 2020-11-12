// errors.hpp
/**\file
 */

#pragma once

#include <boost/system/error_code.hpp>

namespace ss {
using error_code = boost::system::error_code;

namespace error {
enum SessionError {
  Success = 0,
  PartialData, // in buffer contains partial request
  Size,
};

class SessionErrorCategory final : public boost::system::error_category {
public:
  SessionErrorCategory()
      : boost::system::error_category{0x5e3bf594e03e6721} {
  }

  const char *name() const noexcept override {
    return "SessionErrorCategory";
  }

  std::string message(int ev) const override {
    SessionError condition = static_cast<SessionError>(ev);

    switch (condition) {
    case SessionError::Success:
      return "success";
    case SessionError::PartialData:
      return "partial data in request buffer";
    default:
      return "Unkhnown error condition: " + std::to_string(ev);
    }
  }
};

const SessionErrorCategory sessionErrorCategory;

constexpr error_code make_error_code(SessionError ev) noexcept {
  return boost::system::error_code{static_cast<int>(ev), sessionErrorCategory};
}

inline bool
isSessionErrorCategory(const boost::system::error_category &category) noexcept {
  return sessionErrorCategory == category;
}
} // namespace error
} // namespace ss
