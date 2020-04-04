// SessionPool.cpp

#include "ss/SessionPool.hpp"
#include "logs.hpp"
#include "ss/Session.hpp"
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_hash.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <unordered_map>

namespace ss {
namespace uuids   = boost::uuids;
namespace signals = boost::signals2;

class SessionPoolImp {
  enum PoolVal { UnicSession = 0, CloseConnection = 1 };
  using SessionMap = std::unordered_map<
      uuids::uuid,
      std::tuple<std::unique_ptr<Session>, signals::connection>>;

public:
  SessionPoolImp(SessionCloseSignal &atSessionCloseSig)
      : atSessionClose{atSessionCloseSig} {
  }

  /**\return in case of some error return default uuid{00..}
   */
  uuids::uuid append(std::unique_ptr<Session> session) noexcept {
    // generate uuid for the session
    uuids::uuid uuid = gen_();

    // at first connect atClose signal with removing method. Note: it can not be
    // call before calling start method of the session
    signals::connection closeConnection = session->atClose.connect(
        std::bind(&SessionPoolImp::remove, this, uuid));

    auto inserted =
        sessions_.emplace(uuid,
                          std::make_tuple(std::move(session), closeConnection));

    if (!inserted.second) {
      LOG_ERROR("insertion in pool failed");
      return {};
    }

    // and start the session asynchonously
    std::unique_ptr<Session> &insertedSession =
        std::get<PoolVal::UnicSession>(inserted.first->second);
    insertedSession->start();

    LOG_DEBUG("insert session:      %1%", uuid);
    LOG_DEBUG("sessions now opened: %1%", sessions_.size());

    return uuid;
  }

  void remove(const uuids::uuid &uuid) noexcept {
    closed_.clear();
    if (auto found = sessions_.find(uuid); found != sessions_.end()) {
      // XXX we can not remove Session, because in this case we will call
      // destructor of session before it out of signal function.
      // So, we move the session to closed_ pool, which clear every time at
      // beginning of remove call
      closed_.insert(sessions_.extract(found));
      LOG_DEBUG("remove session: %1%", uuid);
      LOG_DEBUG("now opened:     %1%", sessions_.size());

      atSessionClose();
    } else {
      LOG_ERROR("try remove not existing session: %1%", uuid);
    }
  }

  size_t size() const noexcept {
    return sessions_.size();
  }

  void closeAllSessions() noexcept {
    for (auto &[key, poolVal] : sessions_) {
      // first of all we need disconnect close connection
      signals::connection closeConnection =
          std::get<PoolVal::CloseConnection>(poolVal);
      closeConnection.disconnect();

      // and now we can close the session
      std::unique_ptr<Session> &session =
          std::get<PoolVal::UnicSession>(poolVal);
      session->close();
    }

    // after closing all sessions we can clear our map
    this->sessions_.clear();
    this->closed_.clear();

    LOG_DEBUG("current session pool size: %1%", this->sessions_.size());
  }

private:
  uuids::random_generator_mt19937 gen_;
  SessionMap                      sessions_;
  SessionMap                      closed_;

  SessionCloseSignal &atSessionClose;
};

SessionPool::SessionPool() noexcept
    : atSessionClose{}
    , imp_{new SessionPoolImp{this->atSessionClose}} {
}

SessionPool::~SessionPool() noexcept {
  delete imp_;
}

std::string SessionPool::append(std::unique_ptr<Session> session) noexcept {
  uuids::uuid uuid = imp_->append(std::move(session));
  return uuids::to_string(uuid);
}

void SessionPool::remove(const std::string &uuid) noexcept {
  imp_->remove(uuids::string_generator{}(uuid));
}

size_t SessionPool::size() const noexcept {
  return imp_->size();
}

void SessionPool::closeAllSessions() noexcept {
  LOG_DEBUG("stop sessions: %1%", this->size());
  return imp_->closeAllSessions();
}
} // namespace ss
