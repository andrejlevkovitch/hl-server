// AbstractRequestHandler.cpp

#pragma once

#include <boost/system/error_code.hpp>
#include <misc.hpp>
#include <ss/errors.hpp>
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
   * case of partial data you need return SesstionErrors::Partialdata error for
   * reading more.
   */
  virtual error_code handle(const std::string &requestBuffer,
                            OUTPUT std::string &responseBuffer) noexcept = 0;
};
} // namespace ss
