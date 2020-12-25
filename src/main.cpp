// main.cpp

#include "gen/version.hpp"
#include "hl/TokenizeHandlerFactory.hpp"
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/value_semantic.hpp>
#include <boost/program_options/variables_map.hpp>
#include <cstdlib>
#include <simple_logs/logs.hpp>
#include <string>
#include <thread>
#include <vector>

#define HELP_FLAG    "help"
#define VERBOSE_FLAG "verbose"
#define VERSION_FLAG "version"

#define SHORT_VERBOSE_FLAG "v"
#define SHORT_HELP_FLAG    "h"

#define VERSION c_version

#include <boost/stacktrace/stacktrace.hpp>
#include <csignal>

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
  // clang-format off
  options.add_options()
    (HELP_FLAG "," SHORT_HELP_FLAG, "produce help message")
    (VERBOSE_FLAG "," SHORT_VERBOSE_FLAG, "print logs")
    (VERSION_FLAG, "return current version");
  // clang-format on

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
    auto debugFrontend = std::make_shared<logs::LightFrontend>();
    debugFrontend->setFilter(
        logs::Severity::Placeholder == logs::Severity::Trace ||
        logs::Severity::Placeholder == logs::Severity::Info ||
        logs::Severity::Placeholder == logs::Severity::Debug); // all logs

    auto warnFrontend = std::make_shared<logs::LightFrontend>();
    warnFrontend->setFilter(
        logs::Severity::Placeholder == logs::Severity::Warning ||
        logs::Severity::Placeholder == logs::Severity::Throw);

    LOGGER_ADD_SINK(debugFrontend, cerrBackend);
    LOGGER_ADD_SINK(warnFrontend, cerrBackend);
  }


  LOG_INFO("version:  %1%", VERSION);


  // set hander factory for requests
  auto handlerFactory = std::make_shared<hl::TokenizeHandlerFactory>();

  ss::RequestHandler handler = handlerFactory->makeRequestHandler();


  // start request handling
  std::string reqBuffer;
  std::string resBuffer;
  reqBuffer.reserve(1024 * 1024);
  resBuffer.reserve(1024 * 1024);
  for (;;) {
    std::getline(std::cin, reqBuffer);
    if (std::cin.fail()) {
      break;
    }

    // XXX getline ignore `\n`, so we need append it to buffer
    reqBuffer += "\n";
    LOG_DEBUG("readed: %1.3fKb", reqBuffer.size() / 1024.);

    size_t         ignored = 0;
    ss::error_code err;

    err = handler->handle(reqBuffer, std::back_inserter(resBuffer), ignored);
    if (err.failed()) {
      LOG_ERROR(err.message());
    }
    if (ignored != 0 && ignored != reqBuffer.size()) {
      LOG_ERROR("invalid ignored length, expected 0, but has: %1%", ignored);
    }

    std::cout << resBuffer;
    LOG_DEBUG("writed: %1.3fKb", resBuffer.size() / 1024.);

    resBuffer.clear();
  }


  LOG_INFO("exit");
  return EXIT_SUCCESS;
}
