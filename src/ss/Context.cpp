// Context.cpp

#include "ss/Context.hpp"
#include "logs.hpp"
#include <thread>

namespace ss {
static HandlerFactory staticHandlerFactory;

void Context::init() noexcept {
  // initialize asio io_context and mongo instance (they initialize at first
  // call)
  ioContext();
}

asio::io_context &Context::ioContext() noexcept {
  static asio::io_context context;
  return context;
}

void Context::setHandlerFactory(HandlerFactory handlerFactory) noexcept {
  staticHandlerFactory = std::move(handlerFactory);
}

AbstractHandlerFactory *Context::getHandlerFactory() noexcept {
  return staticHandlerFactory.get();
}
} // namespace ss
