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
  if (error_code error = RequestObject::deserialize(request, requestObject);
      error.failed() == true) {
    LOG_ERROR("deserialization error: %1%", error.message());

    // send error message for client
    ResponseObject responseObject{0,
                                  BASE_VERSION_PROTOCOL,
                                  0,
                                  "",
                                  "",
                                  FAILURE_CODE,
                                  error.message(),
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

  ResponseObject responseObject;
  try {
    TokenList tokens = tokenizer->tokenize(requestObject.buf_name,
                                           requestObject.buf_body,
                                           requestObject.additional_info);
    responseObject   = ResponseObject{requestObject.msg_num,
                                    requestObject.version,
                                    requestObject.id,
                                    requestObject.buf_type,
                                    requestObject.buf_name,
                                    SUCCESS_CODE,
                                    "",
                                    std::move(tokens)};
  } catch (std::exception &e) {
    responseObject = ResponseObject{requestObject.msg_num,
                                    requestObject.version,
                                    requestObject.id,
                                    requestObject.buf_type,
                                    requestObject.buf_name,
                                    FAILURE_CODE,
                                    e.what(),
                                    {}};
  }

  if (error_code error = ResponseObject::serialize(responseObject, response);
      error.failed()) {
    // in this case we has serialization logic error, so I don't see any reason
    // for handling it, because client will don't get result. So if you get this
    // error - you must fix logic of serialization
    LOG_FAILURE("invalid serialization: %1%", error.message());
  }

  return returnCode;
}
} // namespace hl
