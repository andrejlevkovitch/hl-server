// TokenizerFactory.cpp

#include "TokenizerFactory.hpp"
#include "CppTokenizer.hpp"
#include "PolyndromTokenizer.hpp"

#define CPP_BUFFER_TYPE "cpp"

#define POLYNDROM_BUFFER_TYPE "polyndrom"

namespace hl::tokenizer {
Tokenizer TokenizerFactory::getTokenizer(std::string_view bufferType) noexcept {
  if (bufferType == CPP_BUFFER_TYPE) {
    return std::make_unique<CppTokenizer>();
  } else if (bufferType == POLYNDROM_BUFFER_TYPE) {
    return std::make_unique<PolyndromTokenizer>();
  } else {
    return Tokenizer{}; // empty tokenizer
  }
}
} // namespace hl::tokenizer
