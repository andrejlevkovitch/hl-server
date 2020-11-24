// TokenizeHandlerFactory.hpp

#pragma once

#include "ss/AbstractRequestHandler.hpp"

namespace hl {
class TokenizeHandlerFactory final : public ss::AbstractRequestHandlerFactory {
public:
  ss::RequestHandler makeRequestHandler() noexcept override;
};
} // namespace hl
