// PolyndromTokenizer.hpp
/**\file simple test example of tokenizer
 */

#pragma once

#include "AbstractTokenizer.hpp"

namespace hl::tokenizer {
class PolyndromTokenizer final : public AbstractTokenizer {
public:
  std::string tokenize(const std::string &bufName,
                       const std::string &buffer,
                       const std::string &additionalInfo,
                       OUTPUT TokenList &tokens) noexcept override;
};
} // namespace hl::tokenizer
