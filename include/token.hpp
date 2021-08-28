#pragma once

#include <array>
#include <list>
#include <string>

namespace hl {
using token_location = std::array<unsigned int, 3>; // row, column, lenght

struct token {
  std::string    group;
  token_location pos;
};

using token_list = std::list<token>;
} // namespace hl
