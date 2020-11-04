// logs.hpp

#include <chrono>

// XXX we don't need log time, so redefine this macro for avoid system call
const std::chrono::time_point<std::chrono::system_clock> time_log_placeholder{};
#define GET_LOG_TIME() time_log_placeholder

#include <simple_logs/logs.hpp>
