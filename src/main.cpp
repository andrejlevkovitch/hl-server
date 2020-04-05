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

#define HOST_ARG       "host"
#define PORT_ARG       "port"
#define LIMIT_SESSIONS "lim_conn"

#define DEFAULT_HOST           "localhost"
#define DEFAULT_PORT           9173
#define DEFAULT_LIMIT_SESSIONS 0

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
      "that capacity of sessions not limited");

  po::variables_map argMap;
  po::store(po::parse_command_line(argc, argv, options), argMap);
  po::notify(argMap);

  std::string host             = argMap[HOST_ARG].as<std::string>();
  uint        port             = argMap[PORT_ARG].as<uint>();
  uint        maxSessionsCount = argMap[LIMIT_SESSIONS].as<uint>();

  LOG_INFO("\nhost: %1%\nport: %2%", host, port);

  ss::HandlerFactory handlerFactory = std::make_unique<hl::HandlerFactory>();
  ss::Context::setHandlerFactory(std::move(handlerFactory));

  ss::Server server{maxSessionsCount};
  server.run(host, port);

  LOG_DEBUG("exit");

  return EXIT_SUCCESS;
}
