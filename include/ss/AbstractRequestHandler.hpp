// AbstractRequestHandler.hpp

#pragma once

#include "ss/errors.hpp"
#include <memory>

namespace ss {
class AbstractRequestHandler {
public:
  using ResponseInserter = std::back_insert_iterator<std::string>;

  virtual ~AbstractRequestHandler() = default;

  virtual ss::error_code atSessionStart() noexcept {
    return boost::system::error_code();
  };

  virtual ss::error_code atSessionClose() noexcept {
    return boost::system::error_code();
  };

  /**\brief handle request and produce responce
   * \param reqIgnoreLength by default is 0. If 0, then request buffer will be
   * completely cleared, otherwise will be cleared directly n-bytes in request
   * buffer. If method return SessionError::PartialData and 0 as reqIgnoreLenght
   * then data in buffer will not be clear
   * \return error_code. If it is not success or not SessionError::PartialData,
   * then session will be closed. If return SessionError::PartialData, then the
   * buffer will be saved and session try read more data to the buffer
   */
  virtual ss::error_code handle(std::string_view requestBuffer,
                                ResponseInserter respIter,
                                size_t &         reqIgnoreLength) noexcept = 0;
};

using RequestHandler = std::shared_ptr<AbstractRequestHandler>;

class AbstractRequestHandlerFactory {
public:
  virtual ~AbstractRequestHandlerFactory()             = default;
  virtual RequestHandler makeRequestHandler() noexcept = 0;
};

using RequestHandlerFactory = std::shared_ptr<AbstractRequestHandlerFactory>;
} // namespace ss
