// ResponseObject.cpp

#include "hl/ResponseObject.hpp"
#include <nlohmann/json.hpp>

#define VERSION_TAG       "version"
#define ID_TAG            "id"
#define BUFFER_TYPE_TAG   "buf_type"
#define BUFFER_NAME_TAG   "buf_name"
#define RETURN_CODE_TAG   "return_code"
#define ERROR_MESSAGE_TAG "error_message"
#define TOKENS_TAG        "tokens"

#define VERSION_PROTOCOL "v1"

namespace hl {
std::string ResponseObject::dump() const noexcept {
  using json = nlohmann::json;

  json serializedTokens = json::object();
  for (const auto &[group, pos] : tokens) {
    serializedTokens[group].emplace_back(pos);
  }

  json retval = json::array();
  retval.push_back(msg_num);
  retval.push_back({
      {VERSION_TAG, version},
      {ID_TAG, id},
      {BUFFER_TYPE_TAG, buf_type},
      {BUFFER_NAME_TAG, buf_name},
      {RETURN_CODE_TAG, return_code},
      {ERROR_MESSAGE_TAG, error_message},
      {TOKENS_TAG, serializedTokens},
  });

  std::string serialized = retval.dump();
  return serialized;
}
} // namespace hl
