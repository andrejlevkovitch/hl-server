// RequestObject.cpp

#include "hl/RequestObject.hpp"
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>
#include <simple_logs/logs.hpp>

namespace hl {
using Json       = nlohmann::json;
namespace system = boost::system;

RequestDeserializer::RequestDeserializer() noexcept
    : validator_{new Validator} {
  validator_->set_root_schema(Json::parse(requestSchema));
}

RequestDeserializer::~RequestDeserializer() noexcept {
  delete validator_;
}

error_code
RequestDeserializer::deserialize(std::string_view request,
                                 OUTPUT RequestObject &reqObj) noexcept {
  // parse request by Protocol signature
  Json        msg = Json::parse(request, nullptr, false);
  std::string errorMsg;
  if (msg == Json::value_t::discarded) {
    errorMsg = "request doesn't contains json object";
    goto Error;
  }

  try {
    validator_->validate(msg);
  } catch (std::exception &e) {
    errorMsg = e.what();
    goto Error;
  }

  reqObj.msg_num = msg[0];

  reqObj.version = msg[1][VERSION_TAG];
  reqObj.id      = msg[1][ID_TAG].is_number()
                  ? std::to_string(msg[1][ID_TAG].get<int>())
                  : msg[1][ID_TAG].get<std::string>();
  reqObj.buf_type        = msg[1][BUFFER_TYPE_TAG];
  reqObj.buf_name        = msg[1][BUFFER_NAME_TAG];
  reqObj.buf_body        = msg[1][BUFFER_BODY_TAG];
  reqObj.additional_info = msg[1][ADDITIONAL_INFO_TAG];

  // clang-format off
[[maybe_unused]]
Success:
  return error_code{};
  // clang-format on

Error:
  LOG_ERROR("invalid message: %1%", errorMsg);

  return system::errc::make_error_code(system::errc::bad_message);
}
} // namespace hl
