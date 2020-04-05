// ResponseObject.cpp

#include "hl/ResponseObject.hpp"
#include <nlohmann/json.hpp>

#define ID_TAG            "id"
#define BUFFER_TYPE_TAG   "buffer_type"
#define BUFFER_NAME_TAG   "buf_name"
#define RETURN_CODE_TAG   "return_code"
#define ERROR_MESSAGE_TAG "error_message"
#define TOKENS_COUNT_TAG  "tokens_count"
#define TOKENS_TAG        "tokens"

#define TOKEN_GROUP_TAG "group"
#define TOKEN_POS_TAG   "pos"

namespace hl {
void ResponseObject::dump(OUTPUT std::string &out) const noexcept {
  using json = nlohmann::json;

  json serializedTokens = json::array();
  std::transform(tokens.begin(),
                 tokens.end(),
                 std::back_inserter(serializedTokens),
                 [](const Token token) {
                   json serializedToken = {{TOKEN_GROUP_TAG, token.group},
                                           {TOKEN_POS_TAG, token.pos}};
                   return serializedToken;
                 });

  json retval = json::array();
  retval.push_back(msg_num);
  retval.push_back({
      {ID_TAG, id},
      {BUFFER_TYPE_TAG, buf_type},
      {BUFFER_NAME_TAG, buf_name},
      {RETURN_CODE_TAG, return_code},
      {ERROR_MESSAGE_TAG, error_message},
      {TOKENS_COUNT_TAG, tokens.size()},
      {TOKENS_TAG, serializedTokens},
  });

  std::string serialized = retval.dump() + '\n';
  std::copy(serialized.begin(), serialized.end(), std::back_inserter(out));
}
} // namespace hl
