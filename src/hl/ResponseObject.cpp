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
using Json = nlohmann::json;

ResponseSerializer::ResponseSerializer() noexcept
    : responseValidator_{nullptr} {
  responseValidator_ = new Validator{};
  responseValidator_->set_root_schema(Json::parse(responseSchema_v1));
}

ResponseSerializer::~ResponseSerializer() noexcept {
  delete responseValidator_;
}

error_code ResponseSerializer::serialize(const ResponseObject &respObj,
                                         OUTPUT std::string &resp) noexcept {
  Json output = Json::array();
  output.emplace_back(respObj.msg_num);

  Json &data = output.emplace_back(Json{
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

  Json &serializedTokens = data[TOKENS_TAG] = Json::object();
  for (const auto &[group, pos] : respObj.tokens) {
    serializedTokens[group].emplace_back(pos);
  }

#ifndef NDEBUG
  RRJsonErrorHandler errorHandler;
  responseValidator_->validate(output, errorHandler);
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
