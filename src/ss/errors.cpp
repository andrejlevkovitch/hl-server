// errors.cpp

#include "ss/errors.hpp"

namespace ss::error {
static const SessionErrorCategory category;

error_code make_error_code(SessionErrors ev) {
  return boost::system::error_code(static_cast<int>(ev), category);
}

bool isSessionErrorCategory(const boost::system::error_category &a_category) {
  return category == a_category;
}
} // namespace ss::error
