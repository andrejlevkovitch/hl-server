// logs.hpp

#include <chrono>

// XXX we don't need log time, so redefine this macro for avoid system call
const std::chrono::time_point<std::chrono::system_clock> time_log_placeholder{};
#define GET_LOG_TIME() time_log_placeholder

#ifdef NDEBUG
#  define LOG_DEBUG(...)
#  define LOG_WARNING(...)

#  define LOG_THROW(ExceptionType, ...)                                        \
    {                                                                          \
      boost::format fmt = logs::messageHandler(__VA_ARGS__);                   \
      throw ExceptionType{fmt.str()};                                          \
    }
#endif

#include <simple_logs/logs.hpp>
