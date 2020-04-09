// RequestObject.cpp

#include "hl/RequestObject.hpp"
#include "logs.hpp"
#include <nlohmann/json.hpp>

#define VERSION_TAG         "version"
#define ID_TAG              "id"
#define BUFFER_TYPE_TAG     "buf_type"
#define BUFFER_NAME_TAG     "buf_name"
#define BUFFER_BODY_TAG     "buf_body"
#define ADDITIONAL_INFO_TAG "additional_info"

#define VERSION_PROTOCOL "v1"

namespace hl {
RequestObject::RequestObject() noexcept
    : msg_num{}
    , version{VERSION_PROTOCOL}
    , id{}
    , buf_type{}
    , buf_name{}
    , buf_body{}
    , additional_info{} {
}

std::string RequestObject::deserialize(std::string_view request) noexcept {
  using json = nlohmann::json;

  // parse request by Protocol signature
  json msg = json::parse(request, nullptr, false);
  if (msg == json::value_t::discarded || msg.is_array() == false ||
      msg.size() != 2) {
    return "invalid request, request must be encoded json array";
  }

  if (msg.at(0).is_number() == false) {
    return "first value in array must be a number";
  }
  if (msg.at(1).is_object() == false) {
    return "second value in array must be an object";
  }

  msg_num    = msg[0];
  json &info = msg[1];

  if (info.contains(VERSION_TAG) == false ||
      info[VERSION_TAG].is_string() == false) {
    return "invalid " VERSION_TAG;
  }
  version = info[VERSION_TAG];
  if (version != VERSION_PROTOCOL) {
    using namespace std::string_literals;
    return "invalid version protocol: "s + version +
           " supported versions: %2%"s + VERSION_PROTOCOL;
  }

  if (info.contains(ID_TAG) == false || info[ID_TAG].is_number() == false) {
    return "invalid " ID_TAG;
  }
  id = info[ID_TAG];

  if (info.contains(BUFFER_TYPE_TAG) == false ||
      info[BUFFER_TYPE_TAG].is_string() == false) {
    return "invalid " BUFFER_TYPE_TAG;
  }
  buf_type = info[BUFFER_TYPE_TAG];

  if (info.contains(BUFFER_NAME_TAG) == false ||
      info[BUFFER_NAME_TAG].is_string() == false) {
    return "invalid " BUFFER_NAME_TAG;
  }
  buf_name = info[BUFFER_NAME_TAG];

  if (info.contains(BUFFER_BODY_TAG) == false ||
      info[BUFFER_BODY_TAG].is_string() == false) {
    return "invalid " BUFFER_BODY_TAG;
  }
  buf_body = info[BUFFER_BODY_TAG];

  if (info.contains(ADDITIONAL_INFO_TAG) == false ||
      info[ADDITIONAL_INFO_TAG].is_string() == false) {
    return "invalid" ADDITIONAL_INFO_TAG;
  }
  additional_info = info[ADDITIONAL_INFO_TAG];

  return "";
}
} // namespace hl
