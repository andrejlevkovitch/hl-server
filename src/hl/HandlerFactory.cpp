// HandlerFactory.cpp

#include "hl/HandlerFactory.hpp"
#include "hl/RequestHandler.hpp"

namespace hl {
ss::Handler HandlerFactory::getRequestHandler() noexcept {
  return std::make_unique<RequestHandler>();
}
} // namespace hl
