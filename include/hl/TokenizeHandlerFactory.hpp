// TokenizeHandlerFactory.hpp

#pragma once

#include "TokenizeHandler.hpp"

namespace ss {
using RequestHandler = std::unique_ptr<hl::TokenizeHandler>;
}

namespace hl {

class TokenizeHandlerFactory final {
public:
  ss::RequestHandler makeRequestHandler() noexcept;
};
} // namespace hl
