// HandlerFactory.hpp

#pragma once

#include "ss/AbstractHandlerFactory.hpp"

namespace hl {
class HandlerFactory final : public ss::AbstractHandlerFactory {
public:
  ss::Handler getRequestHandler() noexcept override;
};
} // namespace hl
