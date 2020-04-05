// PolyndromTokenizer.hpp
/**\file simple test example of tokenizer
 */

#pragma once

#include "AbstractTokenizer.hpp"

namespace hl::tokenizer {
class PolyndromTokenizer final : public AbstractTokenizer {
public:
  error_code tokenize(std::string_view   bufName,
                      const std::string &buffer,
                      OUTPUT TokenList &tokens) noexcept override;
};
} // namespace hl::tokenizer
