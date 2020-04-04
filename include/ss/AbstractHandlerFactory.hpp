// AbstractHandlerFactory.hpp

#pragma once

#include "ss/AbstractRequestHandler.hpp"
#include <memory>

namespace ss {
using Handler = std::unique_ptr<AbstractRequestHander>;

class AbstractHandlerFactory {
public:
  virtual ~AbstractHandlerFactory() = default;

  /**\note the function must be fast, becuase Session call it every time when
   * handle request. This needed for dynamic changing Request handlers
   */
  virtual Handler getRequestHandler() noexcept = 0;
};
} // namespace ss
