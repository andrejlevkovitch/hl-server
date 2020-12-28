// ResponseObject.cpp

#include "hl/ResponseObject.hpp"
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>
#include <simple_logs/logs.hpp>

namespace hl {
using Json       = nlohmann::json;
namespace system = boost::system;

ResponseSerializer::ResponseSerializer() noexcept
    : validator_{new Validator{}} {
  validator_->set_root_schema(Json::parse(responseSchema));
}

ResponseSerializer::~ResponseSerializer() noexcept {
  delete validator_;
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
    data[ID_TAG] = std::stoi(respObj.id);
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

  Json serializedTokens = Json::object();
  for (const auto &[group, pos] : respObj.tokens) {
    serializedTokens[group].emplace_back(pos);
  }
  data[TOKENS_TAG] = std::move(serializedTokens);

#ifndef NDEBUG
  try {
    validator_->validate(output);
  } catch (std::exception &e) {
    LOG_ERROR(e.what());
    return system::errc::make_error_code(system::errc::bad_message);
  }
#endif

  std::string serialized = output.dump();
  std::copy(serialized.begin(), serialized.end(), respInserter);

  return error_code{};
}
} // namespace hl
