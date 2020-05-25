// RequestHandler.cpp

#include "hl/RequestHandler.hpp"
#include "hl/RequestObject.hpp"
#include "hl/ResponseObject.hpp"
#include "logs.hpp"
#include "tokenizer/TokenizerFactory.hpp"
#include <exception>
#include <regex>

#define DATA_DELIMITER        '\n'
#define BASE_VERSION_PROTOCOL "v1.1"

namespace hl {
ss::error_code
RequestHandler::handle(const std::string &requestBuffer,
                       OUTPUT std::string &responseBuffer) noexcept {
  // in this case we has partial data at the end, so just return partial data
  // error
  if (requestBuffer.back() != DATA_DELIMITER) {
    LOG_ERROR("request buffer contains partial data");
    return ss::error::make_error_code(ss::error::SessionErrors::PartialData);
  }

  // XXX because request buffer can contains several requests we need tokenize
  // it and handle only latest. All request before are considered as expired
  static const std::regex    regByDelimiter{DATA_DELIMITER};
  std::sregex_token_iterator requestIterator{requestBuffer.begin(),
                                             requestBuffer.end(),
                                             regByDelimiter,
                                             -1};

  int ignoreRequestCounter = 0;
  for (; std::next(requestIterator) != std::sregex_token_iterator{};
       ++requestIterator, ++ignoreRequestCounter) {
  }

  if (ignoreRequestCounter != 0) {
    LOG_INFO("was ignored %1% requests in buffer", ignoreRequestCounter);
  }

  // and handle latest request
  const std::string &request = requestIterator->str();

  error_code returnCode =
      ss::error::make_error_code(ss::error::SessionErrors::Success);

  RequestObject        requestObject;
  ResponseObject       responseObject;
  tokenizer::Tokenizer tokenizer;
  if (error_code error = RequestObject::deserialize(request, requestObject);
      error.failed() == true) {
    LOG_ERROR("deserialization error: %1%", error.message());

    // send error message for client
    responseObject = ResponseObject{0,
                                    BASE_VERSION_PROTOCOL,
                                    0,
                                    "",
                                    "",
                                    FAILURE_CODE,
                                    error.message(),
                                    {}};
    goto Finally;
  }

  tokenizer = tokenizer::TokenizerFactory::getTokenizer(requestObject.buf_type);
  if (tokenizer == nullptr) {
    LOG_ERROR("couldn't get tokenizer for buffer type: %1%",
              requestObject.buf_type);

    // send error message for client
    responseObject = ResponseObject{requestObject.msg_num,
                                    requestObject.version,
                                    requestObject.id,
                                    requestObject.buf_type,
                                    requestObject.buf_name,
                                    FAILURE_CODE,
                                    "couldn't get tokenizer for buffer type: " +
                                        requestObject.buf_type,
                                    {}};
    goto Finally;
  }

  try {
    TokenList tokens = tokenizer->tokenize(requestObject.buf_type,
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

Finally:
  if (error_code error =
          ResponseObject::serialize(responseObject, responseBuffer);
      error.failed()) {
    // in this case we has serialization logic error, so I don't see any reason
    // for handling it, because client will don't get result. So if you get this
    // error - you must fix logic of serialization
    LOG_FAILURE("invalid serialization: %1%", error.message());
  }

  // responses must be separated by delimiter as requests
  responseBuffer += DATA_DELIMITER;

  return returnCode;
}
} // namespace hl
