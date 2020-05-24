// RequestHandler.hpp

#pragma once

#include "ss/AbstractRequestHandler.hpp"

namespace hl {
/**\brief provide handling Json requests
 * \see hl::RequestObject
 * \see hl::ResponseObject
 */
class RequestHandler final : public ss::AbstractRequestHander {
public:
  /**\note in case of severla requests in one buffer will handle only latest,
   * all requests before are considered as expired
   */
  ss::error_code handle(const std::string &requestBuffer,
                        OUTPUT std::string &responseBuffer) noexcept override;
};
} // namespace hl
