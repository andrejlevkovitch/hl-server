// Context.cpp

#include "ss/Context.hpp"
#include "logs.hpp"
#include <thread>

namespace ss {
static int            numberOfThreads = 1;
static HandlerFactory staticHandlerFactory;

void Context::init(int num) noexcept {
  // XXX context initializes at first call, so we need set number of
  // threads before calling ioContext method
  numberOfThreads = num;
  ioContext();
}

asio::io_context &Context::ioContext() noexcept {
  static asio::io_context context{numberOfThreads};
  return context;
}

void Context::setHandlerFactory(HandlerFactory handlerFactory) noexcept {
  staticHandlerFactory = std::move(handlerFactory);
}

AbstractHandlerFactory *Context::getHandlerFactory() noexcept {
  return staticHandlerFactory.get();
}
} // namespace ss
