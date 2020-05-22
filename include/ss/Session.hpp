// Session.hpp

#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <boost/signals2/signal.hpp>

namespace ss {
namespace asio    = boost::asio;
using tcp         = asio::ip::tcp;
using error_code  = boost::system::error_code;
using CloseSignal = boost::signals2::signal<void()>;

class SessionImp;

/**\brief session represents one conntection to the server
 * \note in case of several requests that was get at one time, will be handle
 * only last request, all reqests before will be ignored - they are considered
 * as expired
 */
class Session {
public:
  Session(tcp::socket sock) noexcept;
  ~Session() noexcept;

  void start() noexcept;

  /**\brief invalidate session
   */
  void close() noexcept;

public:
  /**\note signal emited before destructor call, so you can not imideatly
   * remove session after get the siganl
   *
   * \brief emit after session close
   */
  CloseSignal atClose;

private:
  std::shared_ptr<SessionImp> imp_;
};
} // namespace ss
