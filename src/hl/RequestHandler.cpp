// RequestHandler.cpp

#include "hl/RequestHandler.hpp"
#include "hl/RequestObject.hpp"
#include "hl/ResponseObject.hpp"
#include "logs.hpp"
#include "tokenizer/TokenizerFactory.hpp"
#include <exception>

#define BASE_VERSION_PROTOCOL "v1"

namespace hl {
ss::error_code RequestHandler::handle(std::string_view request,
                                      OUTPUT std::string &response) noexcept {
  ss::error_code returnCode; // always ok

  RequestObject requestObject{};
  if (std::string error = RequestObject::deserialize(request, requestObject);
      error.empty() == false) {
    LOG_ERROR("invalid request: %1%", error);

    // send error message for client
    ResponseObject responseObject{0,
                                  BASE_VERSION_PROTOCOL,
                                  0,
                                  "",
                                  "",
                                  FAILURE_CODE,
                                  error,
                                  {}};
    ResponseObject::serialize(responseObject, response);
    return returnCode;
  }

  tokenizer::Tokenizer tokenizer =
      tokenizer::TokenizerFactory::getTokenizer(requestObject.buf_type);
  if (tokenizer == nullptr) {
    LOG_ERROR("couldn't get tokenizer for buffer type: %1%",
              requestObject.buf_type);

    // send error message for client
    ResponseObject responseObject{requestObject.msg_num,
                                  requestObject.version,
                                  requestObject.id,
                                  requestObject.buf_type,
                                  requestObject.buf_name,
                                  FAILURE_CODE,
                                  "couldn't get tokenizer for buffer type: " +
                                      requestObject.buf_type,
                                  {}};
    ResponseObject::serialize(responseObject, response);
    return returnCode;
  }

  TokenList   tokens;
  std::string status = tokenizer->tokenize(requestObject.buf_name,
                                           requestObject.buf_body,
                                           requestObject.additional_info,
                                           tokens);
  if (status.empty() == false) {
    LOG_WARNING("tokenizer error: %1%", status);
  }

  ResponseObject responseObject{requestObject.msg_num,
                                requestObject.version,
                                requestObject.id,
                                requestObject.buf_type,
                                requestObject.buf_name,
                                status.empty() ? SUCCESS_CODE : FAILURE_CODE,
                                status,
                                std::move(tokens)};
  ResponseObject::serialize(responseObject, response);
  return returnCode;
}
} // namespace hl
