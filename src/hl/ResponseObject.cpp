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

namespace hl {
error_code ResponseObject::serialize(const ResponseObject &respObj,
                                     OUTPUT std::string &resp) noexcept {
  using json = nlohmann::json;

  json output = json::array();
  output.emplace_back(respObj.msg_num);

  json &data = output.emplace_back(json{
      {VERSION_TAG, respObj.version},
      {BUFFER_TYPE_TAG, respObj.buf_type},
      {BUFFER_NAME_TAG, respObj.buf_name},
      {RETURN_CODE_TAG, respObj.return_code},
      {ERROR_MESSAGE_TAG, respObj.error_message},
  });

  std::visit(
      [&data](auto &&arg) {
        data[ID_TAG] = arg;
      },
      respObj.id);

  json &serializedTokens = data[TOKENS_TAG] = json::object();
  for (const auto &[group, pos] : respObj.tokens) {
    serializedTokens[group].emplace_back(pos);
  }

#ifndef NDEBUG
  using validator = nlohmann::json_schema::json_validator;
  validator          vl;
  RRJsonErrorHandler errorHandler;
  vl.set_root_schema(json::parse(responseSchema_v1));
  vl.validate(output, errorHandler);
  if (errorHandler) {
    LOG_ERROR("invalid response serialization: %1%", errorHandler.ss.str());
    return error_code{boost::system::errc::bad_message,
                      boost::system::system_category()};
  }
#endif

  std::string serialized = output.dump();
  std::copy(serialized.begin(), serialized.end(), std::back_inserter(resp));

  return error_code{};
}
} // namespace hl
