// AbstractTokenizer.hpp

#pragma once

#include "hl/Token.hpp"
#include "misc.hpp"
#include <string_view>

namespace hl::tokenizer {
class AbstractTokenizer {
public:
  virtual ~AbstractTokenizer() = default;

  /**\return it can return some string as error or warning, this string just
   * will be added to error_message and tokens also will be passed
   */
  virtual std::string tokenize(const std::string &bufName,
                               const std::string &buffer,
                               const std::string &additionalInfo,
                               OUTPUT TokenList &tokens) noexcept = 0;
};
} // namespace hl::tokenizer
