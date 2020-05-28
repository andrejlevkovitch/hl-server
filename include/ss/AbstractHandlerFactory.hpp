// AbstractHandlerFactory.hpp

#pragma once

#include "ss/AbstractRequestHandler.hpp"
#include <memory>

namespace ss {
using Handler = std::unique_ptr<AbstractRequestHander>;

class AbstractHandlerFactory {
public:
  virtual ~AbstractHandlerFactory() = default;

  /**\brief call at Session start, so every Session has own instance of handler
   */
  virtual Handler getRequestHandler() noexcept = 0;
};
} // namespace ss
