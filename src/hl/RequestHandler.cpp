// RequestHandler.cpp

#include "hl/RequestHandler.hpp"
#include "hl/RequestObject.hpp"
#include "hl/ResponseObject.hpp"
#include "logs.hpp"
#include <exception>

// test
#include <iterator>
#include <list>
#include <regex>

bool is_polyndrom(std::string_view str) {
  std::string reverseString;
  std::copy(str.rbegin(), str.rend(), std::back_inserter(reverseString));

  return str == reverseString;
}

namespace hl {
ss::error_code RequestHandler::handle(std::string_view request,
                                      OUTPUT std::string &response) noexcept {
  ss::error_code returnCode;

  try {
    RequestObject requestObject{request};
    std::string & buffer = requestObject.buf_body;

    // XXX test
    using LineList = std::list<std::string>;

    std::regex newLineReg{"\n"};
    LineList   lines;
    std::copy(std::sregex_token_iterator{buffer.begin(),
                                         buffer.end(),
                                         newLineReg,
                                         -1},
              std::sregex_token_iterator{},
              std::back_inserter(lines));

    std::list<Token> tokens;
    size_t           counter = 0;
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

    ResponseObject responseObject{requestObject.msg_num,
                                  requestObject.id,
                                  requestObject.buf_name,
                                  0,
                                  "",
                                  std::move(tokens)};
    responseObject.dump(response);
  } catch (std::exception &e) {
    LOG_DEBUG("catch exception: %1%", e.what());

    // send error message for client
    ResponseObject responseObject{0, 0, "", 1, e.what(), {}};
    responseObject.dump(response);
  }

  return returnCode;
}
} // namespace hl
