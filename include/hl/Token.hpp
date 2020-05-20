// Token.hpp

#pragma once

#include <array>
#include <list>
#include <string>

namespace hl {
using TokenLocation = std::array<size_t, 3>; // row, column, length

struct Token {
  std::string   group;
  TokenLocation pos;
};

using TokenList = std::list<Token>;
} // namespace hl
