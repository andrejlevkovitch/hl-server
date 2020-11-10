// AbstractRequestHandler.cpp

#pragma once

#include "ss/errors.hpp"
#include <misc.hpp>
#include <string>
#include <string_view>

namespace ss {
class AbstractRequestHander {
public:
  virtual ~AbstractRequestHander() = default;

  /**\return in case of success error code response will be send to client. If
   * error_code is SessionErrors::PartialData, then will be executed another one
   * read operation. Other error codes will close connection
   *
   * \warning if response is empty it handles as error: socket will be closed
   *
   * \note request buffer can contains several requests and/or partial data. In
   * case of partial data you need return SesstionErrors::PartialData error for
   * reading more.
   *
   * \param ignoreLength has a sence if method return PartialData error: if is 0
   * (by default), then all information will be saved in request buffer,
   * otherwise all information from begin to begin + ignoreLength will be ignore
   * and in requestBuffer will be saved only information after begin +
   * ignoreLength
   *
   * \note if ignoreLength bigger then current size of request buffer, then all
   * buffer will clean
   */
  virtual ss::error_code handle(const std::string &requestBuffer,
                                OUTPUT std::string &responseBuffer,
                                OUTPUT size_t &ignoreLength) noexcept = 0;
};
} // namespace ss
