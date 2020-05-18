// CppTokenizer.hpp

#pragma once

#include "AbstractTokenizer.hpp"

namespace hl::tokenizer {
class CppTokenizer : public AbstractTokenizer {
public:
  TokenList tokenize(const std::string &bufName,
                     const std::string &buffer,
                     const std::string &compileFlags) override;
};
} // namespace hl::tokenizer
