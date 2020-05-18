// AbstractTokenizer.hpp

#pragma once

#include "hl/Token.hpp"
#include <string_view>

namespace hl::tokenizer {
class AbstractTokenizer {
public:
  virtual ~AbstractTokenizer() = default;

  virtual TokenList tokenize(const std::string &bufName,
                             const std::string &buffer,
                             const std::string &additionalInfo) = 0;
};
} // namespace hl::tokenizer
