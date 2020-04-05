// CppTokenizer.hpp

#pragma once

#include "AbstractTokenizer.hpp"

namespace hl::tokenizer {
class CppTokenizer : public AbstractTokenizer {
public:
  std::string tokenize(const std::string &bufName,
                       const std::string &buffer,
                       const std::string &compileFlags,
                       OUTPUT TokenList &tokens) noexcept override;
};
} // namespace hl::tokenizer
