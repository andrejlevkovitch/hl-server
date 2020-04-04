// SessionPool.hpp

#pragma once

#include "ss/Session.hpp"
#include <boost/signals2/signal.hpp>
#include <memory>

namespace ss {
class SessionPoolImp;

using SessionCloseSignal = boost::signals2::signal<void()>;

class SessionPool {
public:
  SessionPool() noexcept;
  ~SessionPool() noexcept;

  /**\return uuid of session or empty string if can not insert new session
   * \brief insert new session to pool and start it
   */
  std::string append(std::unique_ptr<Session> session) noexcept;
  void        remove(const std::string &uuid) noexcept;

  size_t size() const noexcept;

  /**\brief close all opened sessions
   */
  void closeAllSessions() noexcept;

public:
  SessionCloseSignal atSessionClose;

private:
  SessionPoolImp *imp_;
};
} // namespace ss
