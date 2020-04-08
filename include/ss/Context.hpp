// Context.hpp

#pragma once

#include "ss/AbstractHandlerFactory.hpp"
#include <boost/asio/io_context.hpp>
#include <string>

namespace ss {
namespace asio = boost::asio;

using HandlerFactory = std::unique_ptr<AbstractHandlerFactory>;

class Context {
public:
  /**\brief you must call this function at first for initialize global context
   *
   * \param num number of threads for context initializing
   */
  static void init(int num) noexcept;

  static asio::io_context &ioContext() noexcept;

  /**\brief for handle requests in Sessions you need implement and set Request
   * handlers and factory for they
   *
   * \see setHandlerFactory
   */
  static AbstractHandlerFactory *getHandlerFactory() noexcept;

  static void setHandlerFactory(HandlerFactory handlerFactory) noexcept;
};
} // namespace ss
