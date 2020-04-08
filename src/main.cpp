// main.cpp

#include "hl/HandlerFactory.hpp"
#include "logs.hpp"
#include "ss/Context.hpp"
#include "ss/Server.hpp"
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/value_semantic.hpp>
#include <boost/program_options/variables_map.hpp>
#include <cstdlib>
#include <thread>
#include <vector>

#define HOST_ARG       "host"
#define PORT_ARG       "port"
#define LIMIT_SESSIONS "lim_conn"
#define THREADS_COUNT  "threads"

#define DEFAULT_HOST           "localhost"
#define DEFAULT_PORT           9173
#define DEFAULT_LIMIT_SESSIONS 0
#define DEFAULT_THREADS_COUNT  1

#include <boost/stacktrace/stacktrace.hpp>
#include <csignal>
void sigsegv_handler([[maybe_unused]] int signal) {
  // XXX note that backtrace printing right only if program compiled with -g
  boost::stacktrace::stacktrace trace{};
  LOG_FAILURE(boost::stacktrace::to_string(trace));
}

int main(int argc, char *argv[]) {
  std::signal(SIGSEGV, sigsegv_handler);

  namespace po = boost::program_options;

  // parse args
  po::options_description options;

  // add options for server
  options.add_options()(
      HOST_ARG,
      po::value<std::string>()->required()->default_value(DEFAULT_HOST),
      "ip for server")(
      PORT_ARG,
      po::value<uint>()->required()->default_value(DEFAULT_PORT),
      "port for server")(
      LIMIT_SESSIONS,
      po::value<uint>()->required()->default_value(DEFAULT_LIMIT_SESSIONS),
      "maximum capacity of opened connections, 0 is special value, which means "
      "that capacity of sessions not limited")(
      THREADS_COUNT,
      po::value<uint>()->required()->default_value(DEFAULT_THREADS_COUNT),
      "capacity of server handling threads");

  po::variables_map argMap;
  po::store(po::parse_command_line(argc, argv, options), argMap);
  po::notify(argMap);

  std::string host             = argMap[HOST_ARG].as<std::string>();
  uint        port             = argMap[PORT_ARG].as<uint>();
  uint        maxSessionsCount = argMap[LIMIT_SESSIONS].as<uint>();
  uint        threadsCount     = argMap[THREADS_COUNT].as<uint>();

  if (threadsCount == 0) {
    LOG_FAILURE("0 is impassible value for count of threads");
  }

  LOG_INFO("\nhost: %1%\nport: %2%\nthreads: %3%", host, port, threadsCount);

  // run context for several threads
  ss::Context::init(threadsCount);
  std::vector<std::thread> threads;
  threads.reserve(threadsCount - 1);

  // set hander factory for requests
  ss::HandlerFactory handlerFactory = std::make_unique<hl::HandlerFactory>();
  ss::Context::setHandlerFactory(std::move(handlerFactory));

  ss::Server server{maxSessionsCount};

  for (size_t threadNum = 0; threadNum < threadsCount - 1; ++threadNum) {
    threads.emplace_back([]() {
      ss::Context::ioContext().run();
    });
  }

  server.run(host, port);
  ss::Context::ioContext().run();

  std::for_each(threads.begin(), threads.end(), [](std::thread &thread) {
    thread.join();
  });

  LOG_DEBUG("exit");

  return EXIT_SUCCESS;
}
