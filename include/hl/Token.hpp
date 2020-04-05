// Token.hpp

#pragma once

#include <array>
#include <list>
#include <string>

namespace hl {
struct Token {
  std::string           group;
  std::array<size_t, 3> pos;
};

using TokenList = std::list<Token>;
} // namespace hl
