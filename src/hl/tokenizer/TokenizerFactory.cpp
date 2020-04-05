// TokenizerFactory.cpp

#include "TokenizerFactory.hpp"
#include "PolyndromTokenizer.hpp"

#define POLYNDROM_BUFFER_TYPE "polyndrom"

namespace hl::tokenizer {
Tokenizer TokenizerFactory::getTokenizer(std::string_view bufferType) noexcept {
  if (bufferType == POLYNDROM_BUFFER_TYPE) {
    return std::make_unique<PolyndromTokenizer>();
  } else {
    return Tokenizer{}; // empty tokenizer
  }
}
} // namespace hl::tokenizer
