// AbstractRequestHandler.cpp

#pragma once

#include <boost/system/error_code.hpp>
#include <misc.hpp>
#include <string>
#include <string_view>

namespace ss {
using error_code = boost::system::error_code;

class AbstractRequestHander {
public:
  virtual ~AbstractRequestHander() = default;

  /**\note in case of error (if return == fail) connection will closed
   *
   * \note response can contains data from previous handling, which was not be
   * send. This happens when server get several request per one time. So be
   * careful, don't clear it - all data must be send to caller
   *
   * \warning if response is empty it handles as error: socket will be closed
   */
  virtual error_code handle(std::string_view reqest,
                            OUTPUT std::string &response) noexcept = 0;
};
} // namespace ss
