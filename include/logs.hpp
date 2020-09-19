// logs.hpp

#include <chrono>

// XXX we don't need log time, so redefine this macro for avoid system call
const std::chrono::time_point<std::chrono::system_clock> time_log_placeholder{};
#define GET_LOG_TIME() time_log_placeholder

#include <simple_logs/logs.hpp>

namespace logs {
class LogLevel final {
public:
};

class CurrentLogger final : public BasicLogger {
public:
  static CurrentLogger &get() {
    static CurrentLogger retval;
    return retval;
  }

  void log(Severity                                           severity,
           std::string_view                                   fileName,
           int                                                lineNumber,
           std::string_view                                   functionName,
           std::chrono::time_point<std::chrono::system_clock> timePoint,
           std::thread::id                                    threadId,
           boost::format message) noexcept override {
    if (severity >= this->getMinLogLevel()) {
      std::cerr << getRecord(severity,
                             fileName,
                             lineNumber,
                             functionName,
                             timePoint,
                             threadId,
                             std::move(message))
                << std::endl;
    }

    if (Severity::Failure == severity) {
      exit(EXIT_FAILURE);
    }
  }

  void setMinLogLevel(logs::Severity min) {
    this->minLogLevel_ = min;
  }

  logs::Severity getMinLogLevel() const {
    return this->minLogLevel_;
  }

private:
  logs::Severity minLogLevel_;
};
} // namespace logs

#undef LOGGER
#define LOGGER logs::CurrentLogger::get()
