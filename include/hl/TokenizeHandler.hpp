// TokenizeHandler.hpp

#pragma once

#include "misc.hpp"
#include "ss/AbstractRequestHandler.hpp"

namespace hl {
class RequestDeserializer;
class ResponseSerializer;

/**\brief provide handling Json requests
 * \see hl::RequestObject
 * \see hl::ResponseObject
 */
class TokenizeHandler final : public ss::AbstractRequestHandler {
public:
  TokenizeHandler() noexcept;
  ~TokenizeHandler() noexcept;

  /**\note in case of several requests in one buffer will handle only latest,
   * all requests before are considered as expired
   */
  ss::error_code
  handle(std::string_view requestBuffer,
         OUTPUT TokenizeHandler::ResponseInserter responseInserter,
         OUTPUT size_t &ignoreLength) noexcept override;

private:
  RequestDeserializer *requestDeserializer_;
  ResponseSerializer * responseSerializer_;
};


class TokenizeHandlerFactory final : public ss::AbstractRequestHandlerFactory {
public:
  ss::RequestHandler makeRequestHandler() noexcept override;
};
} // namespace hl
