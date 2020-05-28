// RequestHandler.hpp

#pragma once

#include "ss/AbstractRequestHandler.hpp"

namespace hl {
class RequestDeserializer;
class ResponseSerializer;

/**\brief provide handling Json requests
 * \see hl::RequestObject
 * \see hl::ResponseObject
 */
class RequestHandler final : public ss::AbstractRequestHander {
public:
  RequestHandler() noexcept;
  ~RequestHandler() noexcept;

  /**\note in case of several requests in one buffer will handle only latest,
   * all requests before are considered as expired
   */
  ss::error_code handle(const std::string &requestBuffer,
                        OUTPUT std::string &responseBuffer) noexcept override;

private:
  RequestDeserializer *requestDeserializer_;
  ResponseSerializer * responseSerializer_;
};
} // namespace hl
