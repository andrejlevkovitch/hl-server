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
  ss::error_code handle(std::string_view request,
                        OUTPUT std::string &response) noexcept override;
};
} // namespace hl
