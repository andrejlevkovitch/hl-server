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

#define VERSION_V1  "v1"
#define VERSION_V11 "v1.1"

namespace hl {
using Json = nlohmann::json;

ResponseSerializer::ResponseSerializer() noexcept
    : responseValidator_1_{nullptr}
    , responseValidator_11_{nullptr} {
  responseValidator_1_ = new Validator{};
  responseValidator_1_->set_root_schema(Json::parse(responseSchema_v1));

  responseValidator_11_ = new Validator{};
  responseValidator_11_->set_root_schema(Json::parse(responseSchema_v11));
}

ResponseSerializer::~ResponseSerializer() noexcept {
  delete responseValidator_1_;
  delete responseValidator_11_;
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

  if (respObj.version == VERSION_V1) { // then we must serialize id as integer
    data[ID_TAG] = std::atoi(respObj.id.c_str());
  } else if (respObj.version == VERSION_V11) {
    data[ID_TAG] = respObj.id;
  } else {
    // XXX if you fail here then you has this version in validator schema, but,
    // for some reason, you don't implement logic for parsing. So, you must fix
    // that
    LOG_FAILURE("not impemented version: %1%", respObj.version);
  }

  Json &serializedTokens = data[TOKENS_TAG] = Json::object();
  for (const auto &[group, pos] : respObj.tokens) {
    serializedTokens[group].emplace_back(pos);
  }

#ifndef NDEBUG
  RRJsonErrorHandler errorHandler;
  if (respObj.version == VERSION_V1) {
    responseValidator_1_->validate(output, errorHandler);
    if (errorHandler) {
      LOG_ERROR("invalid response serialization: %1%", errorHandler.ss.str());
      return error_code{boost::system::errc::bad_message,
                        boost::system::system_category()};
    }
  } else if (respObj.version == VERSION_V11) {
    responseValidator_11_->validate(output, errorHandler);
    if (errorHandler) {
      LOG_ERROR("invalid response serialization: %1%", errorHandler.ss.str());
      return error_code{boost::system::errc::bad_message,
                        boost::system::system_category()};
    }
  } else {
    // XXX if you fail here then you has this version in validator schema,
    // but, for some reason, you don't implement logic for parsing. So, you
    // must fix that
    LOG_ERROR("not impemented version: %1%", respObj.version);
    return error_code{boost::system::errc::bad_message,
                      boost::system::system_category()};
  }
#endif

  std::string serialized = output.dump();
  std::copy(serialized.begin(), serialized.end(), std::back_inserter(resp));

  return error_code{};
}
} // namespace hl
