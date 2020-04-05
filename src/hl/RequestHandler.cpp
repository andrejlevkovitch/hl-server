// RequestHandler.cpp

#include "hl/RequestHandler.hpp"
#include "hl/RequestObject.hpp"
#include "hl/ResponseObject.hpp"
#include "logs.hpp"
#include "tokenizer/TokenizerFactory.hpp"
#include <exception>

namespace hl {
ss::error_code RequestHandler::handle(std::string_view request,
                                      OUTPUT std::string &response) noexcept {
  ss::error_code returnCode;

  RequestObject requestObject;

  try {
    requestObject = RequestObject{request};
  } catch (std::exception &e) {
    LOG_DEBUG("catch exception: %1%", e.what());

    // send error message for client
    ResponseObject responseObject{0, 0, "", "", FAILURE_CODE, e.what(), {}};
    responseObject.dump(response);

    return returnCode;
  }

  tokenizer::Tokenizer tokenizer =
      tokenizer::TokenizerFactory::getTokenizer(requestObject.buf_type);
  if (tokenizer == nullptr) {
    LOG_WARNING("couldn't get tokenizer for buffer type: %1%",
                requestObject.buf_type);

    // send error message for client
    ResponseObject responseObject{requestObject.msg_num,
                                  requestObject.id,
                                  requestObject.buf_type,
                                  requestObject.buf_name,
                                  FAILURE_CODE,
                                  "couldn't get tokenizer for buffer type: " +
                                      requestObject.buf_type,
                                  {}};
    responseObject.dump(response);

    return returnCode;
  }

  TokenList             tokens;
  tokenizer::error_code status = tokenizer->tokenize(requestObject.buf_name,
                                                     requestObject.buf_body,
                                                     tokens);
  if (status.failed()) {
    LOG_WARNING("tokenizer failes");

    // send error message for client
    ResponseObject responseObject{requestObject.msg_num,
                                  requestObject.id,
                                  requestObject.buf_type,
                                  requestObject.buf_name,
                                  FAILURE_CODE,
                                  "tokenizer failes",
                                  tokens};
    responseObject.dump(response);

    return returnCode;
  }

  ResponseObject responseObject{requestObject.msg_num,
                                requestObject.id,
                                requestObject.buf_type,
                                requestObject.buf_name,
                                SUCCESS_CODE,
                                status.message(),
                                std::move(tokens)};
  responseObject.dump(response);

  return returnCode;
}
} // namespace hl
