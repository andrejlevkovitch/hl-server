// TokenizeHandler.hpp

#pragma once

#include "misc.hpp"
#include <boost/system/error_code.hpp>

namespace ss {
using error_code = boost::system::error_code;
}


namespace hl {
class RequestDeserializer;
class ResponseSerializer;

/**\brief provide handling Json requests
 * \see hl::RequestObject
 * \see hl::ResponseObject
 */
class TokenizeHandler final {
public:
  using ResponseInserter = std::back_insert_iterator<std::string>;

  TokenizeHandler() noexcept;
  ~TokenizeHandler() noexcept;

  /**\note in case of several requests in one buffer will handle only latest,
   * all requests before are considered as expired
   */
  ss::error_code
  handle(std::string_view requestBuffer,
         OUTPUT TokenizeHandler::ResponseInserter responseInserter,
         OUTPUT size_t &ignoreLength) noexcept;

private:
  RequestDeserializer *requestDeserializer_;
  ResponseSerializer * responseSerializer_;
};
} // namespace hl
