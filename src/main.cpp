#include "c_arg_parser/arg_parser.h"
#include "c_logs/log.h"
#include "clang_tokenize.hpp"
#include "gen/version.h"
#include "rr_schemes.h"
#include "token.hpp"
#include <arpa/inet.h>
#include <atomic>
#include <csignal>
#include <cstring>
#include <exception>
#include <list>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

#ifdef GO_TOKENIZER
#  include "gotokenizer.h"
#endif

#define ADDRESS   "localhost"
#define BACKLOG   SOMAXCONN
#define BUF_SIZE  1024 * 1024 // 1Mb
#define DELIMITER '\n'


std::atomic_bool done{false};
static void      signal_handler(int val);

static std::string
process(const char *data, int default_flags_count, const char *default_flags[]);


struct connection {
  int         sock;
  sockaddr_in addr;
  size_t      offset;
  char        buf[BUF_SIZE];
};


int main(int argc, char *argv[]) {
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

#ifdef LOGGER_ADD_SYSLOG_SINK
  const char *program_name = strrchr(argv[0], '/');
  program_name = program_name ? program_name + 1 /*ignore slash*/ : argv[0];
  openlog(program_name, LOG_CONS | LOG_PID, 0);
  LOGGER_ADD_SYSLOG_SINK(log_syslog_format,
                         LogFailure | LogError | LogWarning | LogInfo);
#endif

  LOGGER_ADD_STDERR_SINK(log_default_format,
                         LogFailure | LogError | LogWarning | LogInfo);

  arg_parser *parser =
      arg_parser_make("hl-server is a tcp server for c/c++ code tokenization:");

  ARG_PARSER_ADD_BOOL(parser, "help", 'h', "print help", false);
  ARG_PARSER_ADD_BOOLD(parser, "version", 0, "print version", false);
  ARG_PARSER_ADD_BOOLD(parser,
                       "verbose",
                       'v',
                       "print more logs to stderr",
                       false);
  ARG_PARSER_ADD_INTD(parser, "port", 'p', "port for listener", 53827);
  ARG_PARSER_ADD_STR(parser, "root", 0, "set root direcotry", false);
  ARG_PARSER_ADD_STR(parser, "flag", 0, "default compilation flags", false);


  char *       err           = nullptr;
  int          result        = 0;
  bool         need_help     = false;
  bool         need_version  = false;
  bool         need_verbose  = false;
  int          port          = 0;
  const char * root          = NULL;
  int          flag_count    = 0;
  const char **default_flags = NULL;

  int         acceptor = -1;
  sockaddr_in addr;
  int         reuse_addr = 1;

  std::vector<pollfd>     socks;
  std::vector<connection> connections;


  result = ARG_PARSER_PARSE(parser, argc, argv, false, false, &err);
  if (ARG_PARSER_GET_BOOL(parser, "help", need_help) && need_help) {
    goto NeedHelp;
  }
  if (ARG_PARSER_GET_BOOL(parser, "version", need_version) && need_version) {
    goto NeedVersion;
  }
  if (result != 0) {
    LOG_ERROR("error during argument parsing: %s", err);
    LOG_ERROR("please, use -h|--help for help");
    goto Failure;
  }

  ARG_PARSER_GET_BOOL(parser, "verbose", need_verbose);
  if (need_verbose) {
    LOGGER_ADD_STDERR_SINK(log_default_format, LogDebug | LogTrace);
    LOG_INFO("verbose logging is on");
  }

  ARG_PARSER_GET_INT(parser, "port", port);
  LOG_INFO("uses port: %d", port);

  if (ARG_PARSER_GET_STR(parser, "root", root) == 1) {
    LOG_INFO("change root dir to: %s", root);
    if (chroot(root) == -1) {
      LOG_ERROR("can't change root dir: %s", strerror(errno));
    } else {
      LOG_DEBUG("root dir changed")
    }
  }

  LOG_DEBUG("parsing flags")
  flag_count = arg_parser_count(parser, "flag");
  if (flag_count > 0) {
    default_flags = new const char *[flag_count];
    if (arg_parser_get_args(parser,
                            "flag",
                            ArgString,
                            default_flags,
                            flag_count) != flag_count) {
      LOG_FAILURE("can't parse flag values");
    }

    for (int i = 0; i < flag_count; ++i) {
      LOG_DEBUG("default flag: %s", default_flags[i]);
    }
  }
  LOG_DEBUG("flags parsed")


  // resolve address
  LOG_DEBUG("address resolving")
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port   = htons(port);
  result          = inet_aton(ADDRESS, &addr.sin_addr);
  if (result != 0) {
    LOG_ERROR("can't resolve address: %s:%d", ADDRESS, port);
    goto Failure;
  }

  // open socket
  LOG_DEBUG("acceptor opening")
  acceptor = socket(AF_INET, SOCK_STREAM, 0);
  if (acceptor < 0) {
    LOG_ERROR("can't open listener: %s", strerror(errno));
    goto Failure;
  }

  result = setsockopt(acceptor,
                      SOL_SOCKET,
                      SO_REUSEADDR,
                      &reuse_addr,
                      sizeof(reuse_addr));
  if (result != 0) {
    LOG_ERROR("can't set reuse option for listener");
    goto Failure;
  }


  // bind
  LOG_DEBUG("acceptor binding")
  result = bind(acceptor, (struct sockaddr *)&addr, sizeof(addr));
  if (result != 0) {
    LOG_ERROR("can't bind listener: %s", strerror(errno));
    goto Failure;
  }

  // listen
  LOG_DEBUG("acceptor listen")
  result = listen(acceptor, BACKLOG);
  if (result != 0) {
    LOG_ERROR("can't listened: %s", strerror(errno));
    goto Failure;
  }


  socks.reserve(8);
  connections.reserve(8);


  LOG_INFO("start")


  while (done == false) {
    socks.clear();

    // XXX first descriptor in poll is acceptor, always
    pollfd pfd;
    pfd.fd      = acceptor;
    pfd.events  = POLLIN | POLLPRI | POLLERR;
    pfd.revents = 0;
    socks.push_back(pfd);


    for (const connection &con : connections) {
      pfd.fd      = con.sock;
      pfd.events  = POLLIN | POLLPRI | POLLRDHUP | POLLHUP | POLLERR;
      pfd.revents = 0;
      socks.push_back(pfd);
    }


    result = poll(socks.data(), socks.size(), 1000 /*1sec*/);

    if (result < 0) {
      LOG_ERROR("poll error: %s", strerror(errno));
    } else if (result == 0) {
      continue;
    }


    // accept new connection
    if (socks[0].revents != 0) {
      if (socks[0].revents & POLLERR) {
        LOG_ERROR("unexpected acceptor error");
        goto Failure;
      }

      socks[0].revents = 0;


      memset(&addr, 0, sizeof(addr));
      socklen_t sock_len = sizeof(addr);
      int       sock = accept(acceptor, (struct sockaddr *)&addr, &sock_len);
      if (sock < 0) {
        LOG_ERROR("can't accept incomming socket: %s", strerror(errno));
        goto SkipAccepting;
      }

      // append connection
      connections.emplace_back(connection{sock, addr, 0, ""});

      LOG_INFO("accepted connection from %d", ntohs(addr.sin_port));
    }
  SkipAccepting:


    // socket reading
    for (size_t i = 1; i < socks.size(); ++i) {
      assert(i - 1 < connections.size());

      connection &con      = connections[i - 1];
      int         con_port = ntohs(con.addr.sin_port);

      if (socks[i].revents == 0) {
        continue;
      } else if (socks[i].revents & (POLLRDHUP | POLLHUP)) {
        // remove later
        LOG_INFO("connection broken from %d", con_port);
        continue;
      } else if (socks[i].revents & POLLERR) {
        // remove later
        LOG_ERROR("unexpected connection error from %d", con_port);
        continue;
      }
      // otherwise we have data for reading

      socks[i].revents = 0;


      int count =
          read(con.sock, con.buf + con.offset, sizeof(con.buf) - con.offset);
      if (count < 0) {
        // remove later
        socks[i].revents = POLLERR;
        LOG_ERROR("reading error from %d: %s", con_port, strerror(errno))
        continue;
      } else if (count == 0) {
        // remove later
        socks[i].revents = POLLERR;
        continue;
      }

      LOG_DEBUG("readen from %d: %.1fKb", con_port, count / 1024.);

      con.offset += count;
      if (con.buf[con.offset - 1] != DELIMITER) {
        LOG_DEBUG("readen not enough data");

        char *found = strrchr(con.buf, DELIMITER);
        if (found) {
          LOG_DEBUG("ignore old data: %.1fKb",
                    std::distance(con.buf, found) / 1024.);
          strcpy(con.buf, found);
          con.offset = strlen(con.buf);
        } else if (con.offset >= sizeof(con.buf)) {
          LOG_WARNING("too big data, can't store in internal buffer, skip it");
          con.offset = 0;
        }

        continue;
      }


      // get latest data
      char delim              = DELIMITER;
      con.buf[con.offset - 1] = '\0'; // need for ignore latest delimiter
      char *data              = strrchr(con.buf, DELIMITER);
      data                    = data ? data : con.buf;

      LOG_DEBUG_IF(con.buf != data,
                   "ignore old data from %d: %.1fKb",
                   con_port,
                   std::distance(con.buf, data) / 1024.);


      // process
      std::string response = process(data, flag_count, default_flags);


      // switch on cork option
      int cork = 1;
      setsockopt(con.sock, IPPROTO_TCP, TCP_CORK, &cork, sizeof(cork));

      count = write(con.sock, response.c_str(), response.size());
      if (count < 0) {
        // remove later
        socks[i].revents = POLLERR;
        LOG_ERROR("failure during writting response to %d: %s",
                  con_port,
                  strerror(errno));
        continue;
      } else {
        LOG_DEBUG("written: %.1fKb", count / 1024.);
      }

      count = write(con.sock, &delim, 1);
      if (count < 0) {
        // remove later
        socks[i].revents = POLLERR;
        LOG_ERROR("failure during writing delimiter to %d: %s",
                  con_port,
                  strerror(errno));
        continue;
      }

      // switch off cork option
      cork = 0;
      setsockopt(con.sock, IPPROTO_TCP, TCP_CORK, &cork, sizeof(cork));

      // clear buffer
      con.offset = 0;
    }


    // remove all closed and error connections
    size_t counter = 0;
    auto   new_end = std::remove_if(
        connections.begin(),
        connections.end(),
        [&counter, &socks](const connection &con) {
          ++counter;
          if (counter < socks.size() && socks[counter].revents != 0) {
            LOG_INFO("closed connection from %d", ntohs(con.addr.sin_port));
            close(con.sock);
            return true;
          }
          return false;
        });
    connections.erase(new_end, connections.end());
  }


  // finish
  signal(SIGINT, SIG_DFL);
  signal(SIGTERM, SIG_DFL);


  for (connection &con : connections) {
    close(con.sock);
  }
  close(acceptor);


  if (default_flags) {
    delete[] default_flags;
  }

  arg_parser_dispose(parser);

  LOG_INFO("finish");
  LOGGER_SHUTDOWN();
  return EXIT_SUCCESS;

Failure:
  for (connection &con : connections) {
    close(con.sock);
  }
  if (acceptor >= 0) {
    close(acceptor);
  }

  if (err) {
    free(err);
  }
  if (default_flags) {
    delete[] default_flags;
  }
  arg_parser_dispose(parser);
  LOGGER_SHUTDOWN();
  return EXIT_FAILURE;

NeedVersion:
  printf("%s\n", c_version);

  if (err) {
    free(err);
  }
  arg_parser_dispose(parser);
  LOGGER_SHUTDOWN();
  return EXIT_SUCCESS;

NeedHelp:
  char *usage = arg_parser_usage(parser);
  printf("%s", usage);
  free(usage);

  if (err) {
    free(err);
  }
  arg_parser_dispose(parser);
  LOGGER_SHUTDOWN();
  return EXIT_SUCCESS;
}


static void signal_handler(int) {
  done = true;
}

static std::list<std::string> split(const std::string &str) {
  std::list<std::string> retval;

  const char *start = str.c_str();
  do {
    const char *found = strchr(start, DELIMITER);
    if (found) {
      retval.emplace_back(start, found++);
    } else {
      retval.emplace_back(start);
    }

    start = found;
  } while (start);

  return retval;
}

static std::vector<const char *>
to_argv(const std::list<std::string> &string_list) {
  std::vector<const char *> retval;
  retval.reserve(string_list.size());

  for (const std::string &str : string_list) {
    retval.emplace_back(str.c_str());
  }

  return retval;
}

#define VERSION_TAG         "version"
#define ID_TAG              "id"
#define BUF_TYPE_TAG        "buf_type"
#define BUF_NAME_TAG        "buf_name"
#define BUF_BODY_TAG        "buf_body"
#define ADDITIONAL_INFO_TAG "additional_info"
#define RETURN_CODE_TAG     "return_code"
#define ERROR_MESSAGE_TAG   "error_message"
#define TOKENS_TAG          "tokens"

static std::string process(const char *data,
                           int         default_flags_count,
                           const char *default_flags[]) {
  using nlohmann::json;
  using nlohmann::json_schema::json_validator;

  int         message_number = -1;
  std::string version;
  std::string id;
  std::string buf_type;
  std::string buf_name;
  std::string buf_body;
  std::string additional_info;

  json jresponse;

  char        filename[] = ".hl-server-tmp-file-XXXXXX";
  int         fd         = -1;
  int         written    = 0;
  std::string err;

  std::list<std::string>    args;
  std::vector<const char *> argv;
  hl::token_list            tokens;


  try {
    static json schema = json::parse(request_schema_v11);


    json_validator validator;
    validator.set_root_schema(schema);

    json jdata = json::parse(data);
    validator.validate(jdata);

    message_number  = jdata[0];
    version         = jdata[1][VERSION_TAG];
    id              = jdata[1][ID_TAG];
    buf_type        = jdata[1][BUF_TYPE_TAG];
    buf_name        = jdata[1][BUF_NAME_TAG];
    buf_body        = jdata[1][BUF_BODY_TAG];
    additional_info = jdata[1][ADDITIONAL_INFO_TAG];
  } catch (std::exception &e) {
    LOG_ERROR("json handling error: %s", e.what());
    return "";
  }

  jresponse[0]               = message_number;
  jresponse[1][VERSION_TAG]  = version;
  jresponse[1][ID_TAG]       = id;
  jresponse[1][BUF_TYPE_TAG] = buf_type;
  jresponse[1][BUF_NAME_TAG] = buf_name;
  jresponse[1][TOKENS_TAG]   = json::object(); // placeholder


  if (buf_type == "cpp" || buf_type == "c") {
    // create tmp file
    fd = mkstemp(filename);
    if (fd < 0) {
      LOG_ERROR("can't open temporary file: %s", strerror(errno));

      jresponse[1][RETURN_CODE_TAG]   = 2;
      jresponse[1][ERROR_MESSAGE_TAG] = "can't open temporary file for buffer";
      goto Finish;
    }

    written = write(fd, buf_body.c_str(), buf_body.size());
    if (written < 0) {
      LOG_ERROR("can't write buffer to temporary file: %s", strerror(errno));

      jresponse[1][RETURN_CODE_TAG]   = 3;
      jresponse[1][ERROR_MESSAGE_TAG] = "can't write buffer to temporary file";
      goto Finish;
    }


    // tokenization
    args = split(additional_info);
    argv = to_argv(args);
    for (int i = 0; i < default_flags_count; ++i) {
      argv.push_back(default_flags[i]);
    }

    tokens = hl::clang_tokenize(filename, argv.size(), argv.data(), err);
    if (err.empty() == false) {
      LOG_ERROR("error from c/cpp tokenizer: %s", err.c_str());

      jresponse[1][RETURN_CODE_TAG]   = 4;
      jresponse[1][ERROR_MESSAGE_TAG] = "error from tokenizer: " + err;
      goto Finish;
    }

    for (const hl::token &token : tokens) {
      jresponse[1][TOKENS_TAG][token.group].emplace_back(token.pos);
    }
#ifdef GO_TOKENIZER
  } else if (buf_type == "go") {
    char *out  = NULL;
    char *msg  = NULL;
    int   code = 0;

    code = go_tokenize((char *)buf_name.c_str(),
                       (char *)buf_body.c_str(),
                       &out,
                       &msg);

    if (code != 0) {
      LOG_ERROR("error from go tokenizer: %s", msg)

      jresponse[1][RETURN_CODE_TAG] = 4;
      jresponse[1][ERROR_MESSAGE_TAG] =
          "error from tokenizer: " + std::string{msg};
      free(msg);
      goto Finish;
    }

    try {
      jresponse[1][TOKENS_TAG] = json::parse(out);
    } catch (std::exception &e) {
      LOG_ERROR("error during parsing go tokenizer output: %s", e.what());

      jresponse[1][RETURN_CODE_TAG] = 5;
      jresponse[1][ERROR_MESSAGE_TAG] =
          "error during parsing go tokenizer output";

      free(out);
      if (msg) {
        free(msg);
      }
      goto Finish;
    }

    free(out);
    if (msg) {
      LOG_WARNING("warning from go tokenizer: %s", msg)

      free(msg);
    }
#endif
  } else {
    LOG_WARNING("not supported buffer type: %s", buf_type.c_str());

    jresponse[1][RETURN_CODE_TAG]   = 1;
    jresponse[1][ERROR_MESSAGE_TAG] = "unsupported buffer type: " + buf_type;
    goto Finish;
  }


  jresponse[1][RETURN_CODE_TAG]   = 0;
  jresponse[1][ERROR_MESSAGE_TAG] = "";


Finish:
  if (fd > 0) {
    close(fd);
    remove(filename);
  }

#ifndef DNDEBUG
  try {
    static json    response_schema = json::parse(response_schema_v11);
    json_validator response_validator;
    response_validator.set_root_schema(response_schema);
    response_validator.validate(jresponse);
  } catch (std::exception &e) {
    LOG_ERROR("fail validating json response: %s", e.what());
  }
#endif

  return jresponse.dump();
}
