// main.cpp

#include "gen/version.hpp"
#include "hl/TokenizeHandler.hpp"
#include "ss/Server.hpp"
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/value_semantic.hpp>
#include <boost/program_options/variables_map.hpp>
#include <cstdlib>
#include <simple_logs/logs.hpp>
#include <thread>
#include <vector>

#define HELP_FLAG     "help"
#define PORT_FLAG     "port"
#define PROTOCOL_FLAG "protocol"
#define THREADS_COUNT "threads"
#define VERBOSE_FLAG  "verbose"
#define VERSION_FLAG  "version"

#define SHORT_VERBOSE_FLAG "v"
#define SHORT_HELP_FLAG    "h"

#define TCP_PROTOCOL  "tcp"
#define UNIX_PROTOCOL "unix"

#define DEFAULT_PORT          53827
#define DEFAULT_PROTOCOL      TCP_PROTOCOL
#define DEFAULT_THREADS_COUNT 1
#define VERSION               c_version

#include <boost/stacktrace/stacktrace.hpp>
#include <csignal>

namespace asio   = boost::asio;
using error_code = boost::system::error_code;


void sigsegv_handler([[maybe_unused]] int signal) {
  // XXX note that backtrace printing right only if program compiled with -g
  boost::stacktrace::stacktrace trace{};
  LOG_FAILURE(boost::stacktrace::to_string(trace));
}

int main(int argc, char *argv[]) {
  std::signal(SIGSEGV, sigsegv_handler);

  namespace po = boost::program_options;

  // parse args
  po::options_description options{
      "server works only as local server, it can be used by other hosts"};

  // add options for server
  options.add_options()(HELP_FLAG "," SHORT_HELP_FLAG, "produce help message")(
      PORT_FLAG,
      po::value<uint16_t>()->required()->default_value(DEFAULT_PORT),
      "port for server")(
      PROTOCOL_FLAG,
      po::value<std::string>()->required()->default_value(DEFAULT_PROTOCOL),
      "supported protocols: tcp, unix")(
      THREADS_COUNT,
      po::value<uint>()->required()->default_value(DEFAULT_THREADS_COUNT),
      "capacity of server handling threads")(
      VERBOSE_FLAG "," SHORT_VERBOSE_FLAG,
      "print logs")(VERSION_FLAG, "return current version");

  po::variables_map argMap;
  po::store(po::parse_command_line(argc, argv, options), argMap);

  if (argMap.count(HELP_FLAG)) {
    std::cout << options << std::endl;
    return EXIT_SUCCESS;
  }

  if (argMap.count(VERSION_FLAG)) {
    std::cout << VERSION << std::endl;
    return EXIT_SUCCESS;
  }

  po::notify(argMap);


  // init logger
  auto cerrBackend   = std::make_shared<logs::TextStreamBackend>(std::cerr);
  auto errorFrontend = std::make_shared<logs::LightFrontend>();
  errorFrontend->setFilter(logs::Severity::Placeholder >=
                           logs::Severity::Error);
  LOGGER_ADD_SINK(errorFrontend, cerrBackend);

  if (argMap.count(VERBOSE_FLAG)) {
    auto coutBackend   = std::make_shared<logs::TextStreamBackend>(std::cout);
    auto debugFrontend = std::make_shared<logs::LightFrontend>();
    debugFrontend->setFilter(
        logs::Severity::Placeholder == logs::Severity::Trace ||
        logs::Severity::Placeholder == logs::Severity::Info ||
        logs::Severity::Placeholder == logs::Severity::Debug); // all logs

    auto warnFrontend = std::make_shared<logs::LightFrontend>();
    warnFrontend->setFilter(
        logs::Severity::Placeholder == logs::Severity::Warning ||
        logs::Severity::Placeholder == logs::Severity::Throw);

    LOGGER_ADD_SINK(debugFrontend, coutBackend);
    LOGGER_ADD_SINK(warnFrontend, cerrBackend);
  }


  uint        threadsCount = argMap[THREADS_COUNT].as<uint>();
  std::string protocol     = argMap[PROTOCOL_FLAG].as<std::string>();
  uint16_t    port         = argMap[PORT_FLAG].as<uint16_t>();

  // XXX use ONLY localhost
  std::string endpoint = "127.0.0.1:" + std::to_string(port);

  if (threadsCount == 0) {
    LOG_FAILURE("0 is impassible value for count of threads");
  }

  ss::Server::Protocol proto;
  if (protocol == TCP_PROTOCOL) {
    proto = ss::Server::Tcp;
  } else if (protocol == UNIX_PROTOCOL) {
    proto = ss::Server::Unix;
  } else {
    LOG_FAILURE("unsupported protocol: %1%", protocol);
  }

  LOG_INFO("version:  %1%", VERSION);
  LOG_INFO("protocol: %1%", protocol);
  LOG_INFO("endpoint: %1%", endpoint);
  LOG_INFO("threads:  %1%", threadsCount);


  // run context for several threads
  asio::io_context ioContext;

  // set hander factory for requests
  auto handlerFactory = std::make_shared<hl::TokenizeHandlerFactory>();
  ss::ServerBuilder serverBuilder{ioContext};
  serverBuilder.setEndpoint(proto, endpoint)
      .setRequestHandlerFactory(handlerFactory);

  std::shared_ptr<ss::Server> server;
  try {
    server = serverBuilder.build();
  } catch (std::exception &e) {
    LOG_ERROR("exception during server initialization: %s", e.what());
    return EXIT_FAILURE;
  }

  server->asyncRun();


  asio::signal_set sigSet{ioContext, SIGINT, SIGTERM};
  sigSet.async_wait([server, &ioContext](error_code err, int signal) {
    if (err.failed()) {
      LOG_ERROR(err.message());
    }

    switch (signal) {
    case SIGINT:
      LOG_DEBUG("sigint");
      break;
    case SIGTERM:
      LOG_DEBUG("sigint");
      break;
    default:
      LOG_WARNING("unknown signal");
      break;
    }

    server->stop();
    ioContext.stop();
  });


  std::vector<std::thread> threads;
  threads.reserve(threadsCount - 1);
  for (size_t threadNum = 0; threadNum < threadsCount - 1; ++threadNum) {
    threads.emplace_back([&ioContext]() {
      ioContext.run();
    });
  }

  ioContext.run();

  std::for_each(threads.begin(), threads.end(), [](std::thread &thread) {
    thread.join();
  });


  LOG_INFO("exit");
  return EXIT_SUCCESS;
}
