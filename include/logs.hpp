// logs.hpp
/**\file
 * If you want colorized color in terminal then just define `COLORIZED`.
 *
 * Also you can set your own specific format for your message. For do it you
 * need define `STANDARD_LOG_FORMAT` macro as c-string. The string can contains
 * 8 items defined by:
 *
 * - `SEVERITY`
 * - `TERMINAL_COLOR` - specially for logging to terminal
 * - `FILE_NAME`
 * - `LINE_NUMBER`
 * - `FUNCTION_NAME`
 * - `TIME_POINT`
 * - `THREAD_ID`
 * - `MESSAGE`
 *
 * You can combain the defines as you want.
 *
 * Also you must know that after every `TERMINAL_COLOR` must be
 * `TERMINAL_NO_COLOR`, otherwise you can get unexpected colorized output.
 * `TERMINAL_COLOR` accept only if `COLORIZED` defined.
 *
 * \warning Logging macroses produce output only if macro `NDEBUG` not defined,
 * BUT! `LOG_THROW` and `LOG_FAILURE` will work even if `NDEBUG` defined!
 *
 * \warning be carefull in using logging in destructors. Logger is a static
 * object, but there is no guarantie that it will not destroy before some other
 * static objects or etc. If you get error like
 * > pure virtual function called
 * then it can signalized about using logger object after its destruction
 */

#pragma once

#include <boost/format.hpp>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string_view>
#include <thread>

#define INFO_SEVERITY    "INF"
#define DEBUG_SEVERITY   "DBG"
#define WARNING_SEVERITY "WRN"
#define THROW_SEVERITY   "THR"
#define ERROR_SEVERITY   "ERR"
#define FAILURE_SEVERITY "FLR"

// for bash terminal
#ifdef COLORIZED
#  define TERMINAL_NO_COLOR "\033[0m"
#  define TERMINAL_BLUE     "\033[1;34m"
#  define TERMINAL_GREEN    "\033[1;32m"
#  define TERMINAL_YELLOW   "\033[1;33m"
#  define TERMINAL_RED      "\033[1;31m"

#  define COLOR_INFO    TERMINAL_BLUE
#  define COLOR_DEBUG   TERMINAL_GREEN
#  define COLOR_WARNING TERMINAL_YELLOW
#  define COLOR_THROW   TERMINAL_YELLOW
#  define COLOR_ERROR   TERMINAL_RED
#  define COLOR_FAILURE TERMINAL_RED
#else
#  define TERMINAL_NO_COLOR ""

#  define COLOR_INFO    ""
#  define COLOR_DEBUG   ""
#  define COLOR_WARNING ""
#  define COLOR_THROW   ""
#  define COLOR_ERROR   ""
#  define COLOR_FAILURE ""
#endif

// All args set in specific order for formatting
#define SEVERITY       "%1%"
#define TERMINAL_COLOR "%2%"
#define FILE_NAME      "%3%"
#define LINE_NUMBER    "%4%"
#define FUNCTION_NAME  "%5%"
#define TIME_POINT     "%6%"
#define THREAD_ID      "%7%"
#define MESSAGE        "%8%"

#ifndef STANDARD_LOG_FORMAT
/// default logging format
#  define STANDARD_LOG_FORMAT                                                  \
    TERMINAL_COLOR SEVERITY TERMINAL_NO_COLOR                                  \
        ":" FILE_NAME ":" LINE_NUMBER                                          \
        " " TERMINAL_COLOR MESSAGE TERMINAL_NO_COLOR
#endif

namespace std {
/**\brief print time
 */
inline std::ostream &
operator<<(std::ostream &stream,
           const std::chrono::time_point<std::chrono::system_clock>
               &timePoint) noexcept {
  std::time_t t = std::chrono::system_clock::to_time_t(timePoint);
  stream << std::put_time(std::localtime(&t), "%c");
  return stream;
}
} // namespace std

namespace logs {
enum class Severity { Info, Debug, Warning, Error, Failure, Throw };

/**\return safety format object for user message
 */
inline boost::format getLogFormat(std::string_view format) noexcept {
  boost::format retval{std::string{format}};
  retval.exceptions(boost::io::all_error_bits ^ (boost::io::too_few_args_bit |
                                                 boost::io::too_many_args_bit));
  return retval;
}

inline boost::format doFormat(boost::format format) noexcept {
  return format;
}

/**\brief help function for combine all user arguments in one message
 */
template <typename T, typename... Args>
boost::format doFormat(boost::format format, T arg, Args... args) noexcept {
  format % arg;
  return doFormat(std::move(format), args...);
}

inline boost::format messageHandler(std::string_view messageFormat) noexcept {
  boost::format format = getLogFormat(messageFormat);
  return doFormat(std::move(format));
}

/**\brief formatting user message
 */
template <typename... Args>
boost::format messageHandler(std::string_view messageFormat,
                             Args... args) noexcept {
  boost::format format = getLogFormat(messageFormat);
  return doFormat(std::move(format), args...);
}

/**\brief combine message and metadata to one rectord for logging by
 * STANDARD_LOG_FORMAT
 * \see STANDARD_LOG_FORMAT
 */
inline std::string
getRecord(Severity                                           severity,
          std::string_view                                   fileName,
          int                                                lineNumber,
          std::string_view                                   functionName,
          std::chrono::time_point<std::chrono::system_clock> timePoint,
          std::thread::id                                    threadId,
          boost::format                                      message) noexcept {
  boost::format standardLogFormat{STANDARD_LOG_FORMAT};
  standardLogFormat.exceptions(boost::io::all_error_bits ^
                               boost::io::too_many_args_bit);

  switch (severity) {
  case Severity::Info:
    standardLogFormat % INFO_SEVERITY % COLOR_INFO;
    break;
  case Severity::Debug:
    standardLogFormat % DEBUG_SEVERITY % COLOR_DEBUG;
    break;
  case Severity::Warning:
    standardLogFormat % WARNING_SEVERITY % COLOR_WARNING;
    break;
  case Severity::Error:
    standardLogFormat % ERROR_SEVERITY % COLOR_ERROR;
    break;
  case Severity::Failure:
    standardLogFormat % FAILURE_SEVERITY % COLOR_FAILURE;
    break;
  case Severity::Throw:
    standardLogFormat % THROW_SEVERITY % COLOR_THROW;
  }

  standardLogFormat % fileName % lineNumber % functionName % timePoint %
      threadId % message;

  return standardLogFormat.str();
}

class BasicLogger {
public:
  virtual ~BasicLogger() = default;

  virtual void log(Severity         severity,
                   std::string_view fileName,
                   int              lineNumber,
                   std::string_view functionName,
                   std::chrono::time_point<std::chrono::system_clock> timePoint,
                   std::thread::id                                    threadId,
                   boost::format message) noexcept = 0;
};

/**\brief print log messages to terminal
 */
class TerminalLogger final : public BasicLogger {
public:
  void log(Severity                                           severity,
           std::string_view                                   fileName,
           int                                                lineNumber,
           std::string_view                                   functionName,
           std::chrono::time_point<std::chrono::system_clock> timePoint,
           std::thread::id                                    threadId,
           boost::format message) noexcept override {
    std::cerr << getRecord(severity,
                           fileName,
                           lineNumber,
                           functionName,
                           timePoint,
                           threadId,
                           std::move(message))
              << std::endl;

    if (Severity::Failure == severity) {
      exit(EXIT_FAILURE);
    }
  }

  static TerminalLogger &get() noexcept {
    static TerminalLogger logger;
    return logger;
  }

private:
  TerminalLogger() noexcept = default;

  TerminalLogger(const TerminalLogger &) = delete;
  TerminalLogger(TerminalLogger &&)      = delete;
};

class LoggerFactory final {
public:
  static BasicLogger &get() noexcept {
    return TerminalLogger::get();
  }
};
} // namespace logs

/**\brief get current time by system call
 * \warning this is slowly operation, so if you not need logging time you should
 * redefine the macros
 */
#define GET_LOG_TIME() std::chrono::system_clock::now()
/**\brief get current thread id
 */
#define GET_LOG_THREAD_ID() std::this_thread::get_id()

#define LOG_FORMAT(severity, message)                                          \
  logs::LoggerFactory::get().log(severity,                                     \
                                 __FILE__,                                     \
                                 __LINE__,                                     \
                                 __func__,                                     \
                                 GET_LOG_TIME(),                               \
                                 GET_LOG_THREAD_ID(),                          \
                                 message)

#ifdef NDEBUG

#  define LOG_INFO(...)
#  define LOG_DEBUG(...)
#  define LOG_WARNING(...)
#  define LOG_ERROR(...)

#else

#  define LOG_INFO(...)                                                        \
    LOG_FORMAT(logs::Severity::Info, logs::messageHandler(__VA_ARGS__))

#  define LOG_DEBUG(...)                                                       \
    LOG_FORMAT(logs::Severity::Debug, logs::messageHandler(__VA_ARGS__))

#  define LOG_WARNING(...)                                                     \
    LOG_FORMAT(logs::Severity::Warning, logs::messageHandler(__VA_ARGS__))

#  define LOG_ERROR(...)                                                       \
    LOG_FORMAT(logs::Severity::Error, logs::messageHandler(__VA_ARGS__))

#endif

/**\brief print log and terminate program
 * \warning work even if `NDEBUG` defined
 */
#define LOG_FAILURE(...)                                                       \
  LOG_FORMAT(logs::Severity::Failure, logs::messageHandler(__VA_ARGS__))

/**\brief print log and generate specified exception
 * \param ExceptionType type of generated exception
 * \warning work even if `NDEBUG` defined
 */
#define LOG_THROW(ExceptionType, ...)                                          \
  {                                                                            \
    boost::format fmt = logs::messageHandler(__VA_ARGS__);                     \
    LOG_FORMAT(logs::Severity::Throw, fmt);                                    \
    throw ExceptionType{fmt.str()};                                            \
  }
