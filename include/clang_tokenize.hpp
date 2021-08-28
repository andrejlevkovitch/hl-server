#pragma once

#include "token.hpp"
#include <string>


namespace hl {
hl::token_list clang_tokenize(const char * filename,
                              int          argc,
                              const char * argv[],
                              std::string &err) noexcept;
}
