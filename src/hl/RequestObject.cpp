// RequestObject.cpp

#include "hl/RequestObject.hpp"
#include "logs.hpp"
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>

namespace hl {
using Json = nlohmann::json;

RequestDeserializer::RequestDeserializer() noexcept
    : baseRequestValidator_{nullptr}
    , requestValidator_1_{nullptr}
    , requestValidator_11_{nullptr} {
  baseRequestValidator_ = new Validator;
  baseRequestValidator_->set_root_schema(Json::parse(baseReqSchema));

  requestValidator_1_ = new Validator;
  requestValidator_1_->set_root_schema(Json::parse(requestSchema_v1));

  requestValidator_11_ = new Validator;
  requestValidator_11_->set_root_schema(Json::parse(requestSchema_v11));
}

RequestDeserializer::~RequestDeserializer() noexcept {
  delete baseRequestValidator_;
  delete requestValidator_1_;
  delete requestValidator_11_;
}

error_code
RequestDeserializer::deserialize(std::string_view request,
                                 OUTPUT RequestObject &reqObj) noexcept {
  // parse request by Protocol signature
  Json        msg = Json::parse(request, nullptr, false);
  std::string errorMsg;
  if (msg == Json::value_t::discarded) {
    return error_code{boost::system::errc::bad_address,
                      boost::system::system_category()};
  }

  try {
    baseRequestValidator_->validate(msg);
  } catch (std::exception &e) {
    errorMsg = e.what();
    goto Error;
  }

  reqObj.msg_num = msg[0];

  reqObj.version = msg[1][VERSION_TAG];
  switch (getVersionEnum(reqObj.version)) {
  case Version::V1: {
    try {
      requestValidator_1_->validate(msg);
    } catch (std::exception &e) {
      errorMsg = e.what();
      goto Error;
    }

    reqObj.id              = std::to_string(msg[1][ID_TAG].get<int>());
    reqObj.buf_type        = msg[1][BUFFER_TYPE_TAG];
    reqObj.buf_name        = msg[1][BUFFER_NAME_TAG];
    reqObj.buf_body        = msg[1][BUFFER_BODY_TAG];
    reqObj.additional_info = msg[1][ADDITIONAL_INFO_TAG];
    break;
  };
  case Version::V11: {
    try {
      requestValidator_11_->validate(msg);
    } catch (std::exception &e) {
      errorMsg = e.what();
      goto Error;
    }

    reqObj.id              = msg[1][ID_TAG].get<std::string>();
    reqObj.buf_type        = msg[1][BUFFER_TYPE_TAG];
    reqObj.buf_name        = msg[1][BUFFER_NAME_TAG];
    reqObj.buf_body        = msg[1][BUFFER_BODY_TAG];
    reqObj.additional_info = msg[1][ADDITIONAL_INFO_TAG];
    break;
  };
  default:
    // XXX if you fail here then you has this version in validator schema, but,
    // for some reason, you don't implement logic for parsing. So, you must fix
    // that
    errorMsg = "not impemented version: " + reqObj.version;
    goto Error;
  }

  // clang-format off
[[maybe_unused]] Success:
  return error_code{};
  // clang-format on

Error:
  LOG_ERROR("invalid message: %1%", errorMsg);

  return error_code{boost::system::errc::bad_message,
                    boost::system::system_category()};
}
} // namespace hl
