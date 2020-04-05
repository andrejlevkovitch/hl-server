// PolyndromTokenizer.cpp

#include "PolyndromTokenizer.hpp"
#include <regex>

namespace hl::tokenizer {
bool is_polyndrom(std::string_view str) {
  std::string reverseString;
  std::copy(str.rbegin(), str.rend(), std::back_inserter(reverseString));

  return str == reverseString;
}

std::string
PolyndromTokenizer::tokenize([[maybe_unused]] const std::string &bufName,
                             const std::string &                 buffer,
                             [[maybe_unused]] const std::string &additionalInfo,
                             OUTPUT TokenList &tokens) noexcept {
  using LineList = std::list<std::string>;

  std::regex newLineReg{"\n"};
  LineList   lines;
  std::copy(
      std::sregex_token_iterator{buffer.begin(), buffer.end(), newLineReg, -1},
      std::sregex_token_iterator{},
      std::back_inserter(lines));

  size_t counter = 0;
  for (const std::string &line : lines) {
    std::regex wordReg{R"(\w+)"};
    for (auto iter =
             std::sregex_token_iterator{line.begin(), line.end(), wordReg};
         iter != std::sregex_token_iterator{};
         ++iter) {
      std::string word = iter->str();
      if (is_polyndrom(word)) {
        // XXX numberation for rows and columns start from 1
        size_t      row    = counter + 1;
        size_t      column = std::distance(line.begin(), iter->first) + 1;
        size_t      length = iter->length();
        std::string group  = "Label";
        tokens.push_back(Token{group, {row, column, length}});
      }
    }

    ++counter;
  }

  return {};
}
} // namespace hl::tokenizer
