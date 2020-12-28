// TokenizeHandler.cpp

#include "hl/TokenizeHandler.hpp"
#include "hl/RequestObject.hpp"
#include "hl/ResponseObject.hpp"
#include "tokenizer/TokenizerFactory.hpp"
#include <exception>
#include <regex>
#include <simple_logs/logs.hpp>

#define DATA_DELIMITER '\n'

namespace hl {
TokenizeHandler::TokenizeHandler() noexcept
    : requestDeserializer_{nullptr}
    , responseSerializer_{nullptr} {
  requestDeserializer_ = new RequestDeserializer{};
  responseSerializer_  = new ResponseSerializer{};
}

TokenizeHandler::~TokenizeHandler() noexcept {
  delete requestDeserializer_;
  delete responseSerializer_;
}

ss::error_code TokenizeHandler::handle(
    std::string_view requestBuffer,
    OUTPUT TokenizeHandler::ResponseInserter responseInserter,
    OUTPUT size_t &ignoreLength) noexcept {
  // XXX because request buffer can contains several requests we need tokenize
  // it and handle only latest. All request before are considered as expired
  static const std::regex    regByDelimiter{DATA_DELIMITER};
  std::cregex_token_iterator requestIterator{requestBuffer.begin(),
                                             requestBuffer.end(),
                                             regByDelimiter,
                                             -1};

  int ignoreRequestCounter = 0;
  for (; std::next(requestIterator) != std::cregex_token_iterator{};
       ++requestIterator, ++ignoreRequestCounter) {
  }

  if (ignoreRequestCounter != 0) {
    LOG_WARNING("was ignored %1% requests in buffer", ignoreRequestCounter);
  }

  // in this case we has partial data at the end, so just return partial data
  // error
  if (requestBuffer.back() != DATA_DELIMITER) {
    LOG_WARNING("request buffer contains partial data");

    ignoreLength = std::distance(requestBuffer.begin(), requestIterator->first);
    return ss::error::make_error_code(ss::error::SessionError::PartialData);
  }

  // and handle latest request
  const std::string &request = requestIterator->str();

  error_code returnCode =
      ss::error::make_error_code(ss::error::SessionError::Success);

  RequestObject        requestObject;
  ResponseObject       responseObject;
  tokenizer::Tokenizer tokenizer;
  if (error_code error =
          requestDeserializer_->deserialize(request, requestObject);
      error.failed() == true) {
    LOG_ERROR("deserialization error: %1%", error.message());

    // send error message for client
    responseObject = ResponseObject{0,
                                    BASE_VERSION_PROTOCOL,
                                    "",
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

    responseObject = ResponseObject{requestObject.msg_num,
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
          responseSerializer_->serialize(responseObject, responseInserter);
      error.failed()) {
    // in this case we has serialization logic error, so I don't see any reason
    // for handling it, because client will don't get result. So if you get this
    // error - you must fix logic of serialization
    LOG_FAILURE("invalid serialization: %1%", error.message());
  }

  // responses must be separated by delimiter as requests
  responseInserter = DATA_DELIMITER;

  return returnCode;
}


ss::RequestHandler TokenizeHandlerFactory::makeRequestHandler() noexcept {
  return std::make_unique<TokenizeHandler>();
}
} // namespace hl
