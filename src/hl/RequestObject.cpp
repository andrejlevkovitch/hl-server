// RequestObject.cpp

#include "hl/RequestObject.hpp"
#include "logs.hpp"
#include <nlohmann/json.hpp>

#define ID_TAG              "id"
#define BUFFER_TYPE_TAG     "buf_type"
#define BUFFER_NAME_TAG     "buf_name"
#define BUFFER_BODY_TAG     "buf_body"
#define ADDITIONAL_INFO_TAG "additional_info"

namespace hl {
RequestObject::RequestObject(std::string_view request) {
  using json = nlohmann::json;

  // parse request by Protocol signature
  json msg = json::parse(request, nullptr, false);
  if (msg == json::value_t::discarded || msg.is_array() == false ||
      msg.size() != 2) {
    LOG_ERROR("\n%1%", request);
    LOG_THROW(std::invalid_argument,
              "invalid request, request must be encoded json array");
  }

  if (msg.at(0).is_number() == false) {
    LOG_THROW(std::invalid_argument, "first value in array must be a number");
  }
  if (msg.at(1).is_object() == false) {
    LOG_THROW(std::invalid_argument, "second value in array must be an object");
  }

  msg_num   = msg[0];
  json info = msg[1];

  if (info.contains(ID_TAG) == false || info[ID_TAG].is_number() == false) {
    LOG_THROW(std::invalid_argument, "invalid " ID_TAG);
  }
  if (info.contains(BUFFER_TYPE_TAG) == false ||
      info[BUFFER_TYPE_TAG].is_string() == false) {
    LOG_THROW(std::invalid_argument, "invalid " BUFFER_TYPE_TAG);
  }
  if (info.contains(BUFFER_NAME_TAG) == false ||
      info[BUFFER_NAME_TAG].is_string() == false) {
    LOG_THROW(std::invalid_argument, "invalid " BUFFER_NAME_TAG);
  }
  if (info.contains(BUFFER_BODY_TAG) == false ||
      info[BUFFER_BODY_TAG].is_string() == false) {
    LOG_THROW(std::invalid_argument, "invalid " BUFFER_BODY_TAG);
  }
  if (info.contains(ADDITIONAL_INFO_TAG) == false ||
      info[ADDITIONAL_INFO_TAG].is_string() == false) {
    LOG_THROW(std::invalid_argument, "invalid" ADDITIONAL_INFO_TAG);
  }

  id              = info[ID_TAG];
  buf_type        = info[BUFFER_TYPE_TAG];
  buf_name        = info[BUFFER_NAME_TAG];
  buf_body        = info[BUFFER_BODY_TAG];
  additional_info = info[ADDITIONAL_INFO_TAG];
}
} // namespace hl
