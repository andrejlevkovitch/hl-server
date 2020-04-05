// TokenizerFactory.hpp

#pragma once

#include "AbstractTokenizer.hpp"
#include <memory>
#include <string_view>

namespace hl::tokenizer {
using Tokenizer = std::unique_ptr<AbstractTokenizer>;

class TokenizerFactory final {
public:
  /**\note return empty Tokenizer object if couldn't find tokenizer with needed
   * type
   */
  static Tokenizer getTokenizer(std::string_view bufferType) noexcept;
};
} // namespace hl::tokenizer
