// HandlerFactory.cpp

#include "hl/TokenizeHandlerFactory.hpp"
#include "hl/TokenizeHandler.hpp"

namespace hl {
ss::RequestHandler TokenizeHandlerFactory::makeRequestHandler() noexcept {
  return std::make_unique<TokenizeHandler>();
}
} // namespace hl
