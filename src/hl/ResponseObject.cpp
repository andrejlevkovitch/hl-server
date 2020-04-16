// ResponseObject.cpp

#include "hl/ResponseObject.hpp"
#include "RRJsonErrorHandler.hpp"
#include "logs.hpp"
#include <nlohmann/json-schema.hpp>
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

  json msg = json::array();
  msg.push_back(msg_num);
  msg.push_back({
      {VERSION_TAG, version},
      {ID_TAG, id},
      {BUFFER_TYPE_TAG, buf_type},
      {BUFFER_NAME_TAG, buf_name},
      {RETURN_CODE_TAG, return_code},
      {ERROR_MESSAGE_TAG, error_message},
      {TOKENS_TAG, serializedTokens},
  });

#ifndef NDEBUG
  using validator = nlohmann::json_schema::json_validator;
  validator          vl;
  RRJsonErrorHandler errorHandler;
  vl.set_root_schema(json::parse(responseSchema_v1));
  vl.validate(msg, errorHandler);
  if (errorHandler) {
    LOG_FAILURE("invalid response serialization: %1%", errorHandler.ss.str());
  }
#endif

  std::string serialized = msg.dump();
  return serialized;
}
} // namespace hl
