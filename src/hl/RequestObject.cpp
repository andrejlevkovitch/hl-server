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
RequestObject::RequestObject() noexcept
    : msg_num{}
    , version{}
    , id{}
    , buf_type{}
    , buf_name{}
    , buf_body{}
    , additional_info{} {
}

std::string RequestObject::deserialize(std::string_view request) noexcept {
  using json      = nlohmann::json;
  using validator = nlohmann::json_schema::json_validator;

  // parse request by Protocol signature
  json msg = json::parse(request, nullptr, false);
  if (msg == json::value_t::discarded) {
    return "invalid request, request must be encoded json array";
  }

  validator          vl;
  RRJsonErrorHandler errorHandler;
  vl.set_root_schema(json::parse(requestSchema_v1));
  vl.validate(msg, errorHandler);

  if (errorHandler == false) {
    msg_num    = msg[0];
    json &info = msg[1];

    version         = info[VERSION_TAG];
    id              = info[ID_TAG];
    buf_type        = info[BUFFER_TYPE_TAG];
    buf_name        = info[BUFFER_NAME_TAG];
    buf_body        = info[BUFFER_BODY_TAG];
    additional_info = info[ADDITIONAL_INFO_TAG];

    return "";
  } else {
    std::string errors = errorHandler.ss.str();
    return errors;
  }
}
} // namespace hl
