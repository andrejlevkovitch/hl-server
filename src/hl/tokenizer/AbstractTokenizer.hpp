// AbstractTokenizer.hpp

#pragma once

#include "hl/Token.hpp"
#include "misc.hpp"
#include <boost/system/error_code.hpp>
#include <string_view>

namespace hl::tokenizer {
using error_code = boost::system::error_code;

class AbstractTokenizer {
public:
  virtual ~AbstractTokenizer() = default;

  virtual error_code tokenize(std::string_view   bufName,
                              const std::string &buffer,
                              OUTPUT TokenList &tokens) noexcept = 0;
};
} // namespace hl::tokenizer
