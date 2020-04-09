// RequestHandler.cpp

#include "hl/RequestHandler.hpp"
#include "hl/RequestObject.hpp"
#include "hl/ResponseObject.hpp"
#include "logs.hpp"
#include "tokenizer/TokenizerFactory.hpp"
#include <exception>

namespace hl {
static void writeResponse(const ResponseObject &responseObject,
                          OUTPUT std::string &response) noexcept {
  std::string serializedResponse = responseObject.dump();

  std::copy(serializedResponse.begin(),
            serializedResponse.end(),
            std::back_inserter(response));
}

ss::error_code RequestHandler::handle(std::string_view request,
                                      OUTPUT std::string &response) noexcept {
  ss::error_code returnCode; // always ok

  std::string   serializedResponse;
  RequestObject requestObject{};

  std::string error = requestObject.deserialize(request);
  if (error.empty() == false) {
    LOG_WARNING("catch exception: %1%", error);

    // send error message for client
    ResponseObject responseObject{requestObject.msg_num,
                                  requestObject.version,
                                  requestObject.id,
                                  requestObject.buf_type,
                                  requestObject.buf_name,
                                  FAILURE_CODE,
                                  error,
                                  {}};
    writeResponse(responseObject, response);
    return returnCode;
  }

  tokenizer::Tokenizer tokenizer =
      tokenizer::TokenizerFactory::getTokenizer(requestObject.buf_type);
  if (tokenizer == nullptr) {
    LOG_WARNING("couldn't get tokenizer for buffer type: %1%",
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
    writeResponse(responseObject, response);
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
  writeResponse(responseObject, response);
  return returnCode;
}
} // namespace hl
