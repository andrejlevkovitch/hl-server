// RequestObject.cpp

#include "hl/RequestObject.hpp"
#include "RRJsonErrorHandler.hpp"
#include "logs.hpp"
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>

#define VERSION_TAG         "version"
#define ID_TAG              "id"
#define BUFFER_TYPE_TAG     "buf_type"
#define BUFFER_NAME_TAG     "buf_name"
#define BUFFER_BODY_TAG     "buf_body"
#define ADDITIONAL_INFO_TAG "additional_info"

#define VERSION_V1  "v1"
#define VERSION_V11 "v1.1"

namespace hl {
using Json = nlohmann::json;

static const Json baseReqSchema = Json::parse(R"(
{
  "type": "array",
  "items": [
    { "$ref": "#/definitions/message_number" },
    {"$ref": "#/definitions/request_body"}
  ],
  "minItems": 2,
  "maxItems": 2,
  "definitions": {
    "message_number": {
      "type": "integer"
    },
    "request_body": {
      "type": "object",
      "required": [
          "version"
      ],
      "properties": {
        "version": {
          "type": "string",
          "enum": ["v1", "v1.1"]
        }
      }
    }
  }
}
)");

RequestDeserializer::RequestDeserializer() noexcept
    : baseRequestValidator_{nullptr}
    , requestValidator_1_{nullptr}
    , requestValidator_11_{nullptr} {
  baseRequestValidator_ = new Validator;
  baseRequestValidator_->set_root_schema(baseReqSchema);

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
  Json msg = Json::parse(request, nullptr, false);
  if (msg == Json::value_t::discarded) {
    return error_code{boost::system::errc::bad_address,
                      boost::system::system_category()};
  }

  RRJsonErrorHandler errorHandler;
  baseRequestValidator_->validate(msg, errorHandler);

  if (errorHandler == true) {
    goto Error;
  }

  reqObj.msg_num = msg[0];

  reqObj.version = msg[1][VERSION_TAG];
  if (reqObj.version == VERSION_V1) {
    requestValidator_1_->validate(msg, errorHandler);

    if (errorHandler == true) {
      goto Error;
    }

    reqObj.id              = std::to_string(msg[1][ID_TAG].get<int>());
    reqObj.buf_type        = msg[1][BUFFER_TYPE_TAG];
    reqObj.buf_name        = msg[1][BUFFER_NAME_TAG];
    reqObj.buf_body        = msg[1][BUFFER_BODY_TAG];
    reqObj.additional_info = msg[1][ADDITIONAL_INFO_TAG];
  } else if (reqObj.version == VERSION_V11) {
    requestValidator_11_->validate(msg, errorHandler);

    if (errorHandler == true) {
      goto Error;
    }

    reqObj.id              = msg[1][ID_TAG].get<std::string>();
    reqObj.buf_type        = msg[1][BUFFER_TYPE_TAG];
    reqObj.buf_name        = msg[1][BUFFER_NAME_TAG];
    reqObj.buf_body        = msg[1][BUFFER_BODY_TAG];
    reqObj.additional_info = msg[1][ADDITIONAL_INFO_TAG];
  } else {
    // XXX if you fail here then you has this version in validator schema, but,
    // for some reason, you don't implement logic for parsing. So, you must fix
    // that
    errorHandler.ss << "not impemented version: " << reqObj.version;
    goto Error;
  }

  // Success:
  return error_code{};

Error:
  std::string errors = errorHandler.ss.str();
  LOG_ERROR("invalid message: %1%", errors);

  return error_code{boost::system::errc::bad_message,
                    boost::system::system_category()};
}
} // namespace hl
