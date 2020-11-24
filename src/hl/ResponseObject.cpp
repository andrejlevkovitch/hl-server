// ResponseObject.cpp

#include "hl/ResponseObject.hpp"
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>
#include <simple_logs/logs.hpp>

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

error_code ResponseSerializer::serialize(
    const ResponseObject &respObj,
    OUTPUT std::back_insert_iterator<std::string> respInserter) noexcept {
  Json output = Json::array();
  output.emplace_back(respObj.msg_num);

  Json &data = output.emplace_back(Json{
      {VERSION_TAG, respObj.version},
      {BUFFER_TYPE_TAG, respObj.buf_type},
      {BUFFER_NAME_TAG, respObj.buf_name},
      {RETURN_CODE_TAG, respObj.return_code},
      {ERROR_MESSAGE_TAG, respObj.error_message},
  });

  switch (getVersionEnum(respObj.version)) {
  case Version::V1:
    data[ID_TAG] = std::atoi(respObj.id.c_str());
    break;
  case Version::V11:
    data[ID_TAG] = respObj.id;
    break;
  default:
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
  try {
    switch (getVersionEnum(respObj.version)) {
    case Version::V1:
      responseValidator_1_->validate(output);
      break;
    case Version::V11:
      responseValidator_11_->validate(output);
      break;
    default:
      // XXX if you fail here then you has this version in validator schema,
      // but, for some reason, you don't implement logic for parsing. So, you
      // must fix that
      LOG_THROW(std::invalid_argument,
                "invalid protocol version: %1%",
                respObj.version);
    }
  } catch (std::exception &e) {
    LOG_ERROR(e.what());
    return error_code{boost::system::errc::bad_message,
                      boost::system::system_category()};
  }
#endif

  std::string serialized = output.dump();
  std::copy(serialized.begin(), serialized.end(), respInserter);

  return error_code{};
}
} // namespace hl
