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

namespace hl {
using Json = nlohmann::json;

RequestDeserializer::RequestDeserializer() noexcept
    : requestValidator_{nullptr} {
  requestValidator_ = new Validator;
  requestValidator_->set_root_schema(Json::parse(requestSchema_v1));
}

RequestDeserializer::~RequestDeserializer() noexcept {
  delete requestValidator_;
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
  requestValidator_->validate(msg, errorHandler);

  if (errorHandler == false) {
    reqObj.msg_num = msg[0];
    Json &info     = msg[1];

    reqObj.version = info[VERSION_TAG];
    if (info.type() == Json::value_t::number_integer) {
      reqObj.id = info[ID_TAG].get<int>();
    } else {
      reqObj.id = info[ID_TAG].get<std::string>();
    }
    reqObj.buf_type        = info[BUFFER_TYPE_TAG];
    reqObj.buf_name        = info[BUFFER_NAME_TAG];
    reqObj.buf_body        = info[BUFFER_BODY_TAG];
    reqObj.additional_info = info[ADDITIONAL_INFO_TAG];

    return error_code{};
  } else {
    std::string errors = errorHandler.ss.str();
    LOG_ERROR("invalid message: %1%", errors);

    return error_code{boost::system::errc::bad_message,
                      boost::system::system_category()};
  }
}
} // namespace hl
