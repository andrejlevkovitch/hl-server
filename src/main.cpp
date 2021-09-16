#include "c_arg_parser/arg_parser.h"
#include "c_logs/log.h"
#include "clang_tokenize.hpp"
#include "gen/version.h"
#include "rr_schemes.h"
#include "token.hpp"
#include <arpa/inet.h>
#include <atomic>
#include <csignal>
#include <iostream>
#include <list>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>


#define ADDRESS   "localhost"
#define BACKLOG   SOMAXCONN
#define BUF_SIZE  1024 * 1024 // 1Mb
#define DELIMITER '\n'


std::atomic_bool done{false};
static void      signal_handler(int val);

static std::string
process(const char *data, int default_flags_count, const char *default_flags[]);

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

  int         sock = -1;
  sockaddr_in addr;
  int         in_sock = -1;
  sockaddr_in in_addr;
  socklen_t   sock_len   = 0;
  int         reuse_addr = 1;
  timeval     rcv_timeout;

  std::list<pid_t> children;


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
    }
  }

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


  // resolve address
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port   = htons(port);
  result          = inet_aton(ADDRESS, &addr.sin_addr);
  if (result != 0) {
    LOG_ERROR("can't resolve address: %s:%d", ADDRESS, port);
    goto Failure;
  }

  // open socket
  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    LOG_ERROR("can't open listener: %s", strerror(errno));
    goto Failure;
  }

  result = setsockopt(sock,
                      SOL_SOCKET,
                      SO_REUSEADDR,
                      &reuse_addr,
                      sizeof(reuse_addr));
  if (result != 0) {
    LOG_ERROR("can't set reuse option for listener");
    goto Failure;
  }

  rcv_timeout.tv_sec  = 1;
  rcv_timeout.tv_usec = 0;

  result = setsockopt(sock,
                      SOL_SOCKET,
                      SO_RCVTIMEO,
                      &rcv_timeout,
                      sizeof(rcv_timeout));
  if (result != 0) {
    LOG_ERROR("can't set timeout for listener");
    goto Failure;
  }

  // bind
  result = bind(sock, (struct sockaddr *)&addr, sizeof(addr));
  if (result != 0) {
    LOG_ERROR("can't bind listener: %s", strerror(errno));
    goto Failure;
  }

  // listen
  result = listen(sock, BACKLOG);
  if (result != 0) {
    LOG_ERROR("can't listened: %s", strerror(errno));
    goto Failure;
  }

  while (done == false) {
    // accept new connection
    sock_len = sizeof(in_addr);
    in_sock  = accept(sock, (struct sockaddr *)&in_addr, &sock_len);
    if (in_sock < 0 && errno == EAGAIN) {
      continue;
    } else if (in_sock < 0) {
      LOG_ERROR("can't accept incomming socket: %s", strerror(errno));
      continue;
    }

    LOG_INFO("accepted connection from port: %d", ntohs(in_addr.sin_port));

    pid_t pid = fork();
    if (pid == -1) {
      close(in_sock);
      LOG_ERROR("error during fork: %s", strerror(errno));
      continue;
    } else if (pid > 0) {
      LOG_INFO("start child process: %d", pid);
      children.emplace_back(pid);
      continue;
    }

    // child
    rcv_timeout.tv_sec  = 1;
    rcv_timeout.tv_usec = 0;

    result = setsockopt(in_sock,
                        SOL_SOCKET,
                        SO_RCVTIMEO,
                        &rcv_timeout,
                        sizeof(rcv_timeout));
    if (result != 0) {
      LOG_ERROR("can't set timeout for input socket");
    }


    int      count  = 0;
    unsigned offset = 0;
    char     buf[BUF_SIZE];
    while (done == false) {
      if (offset == sizeof(buf) - 1) {
        LOG_WARNING(
            "ignore %.1fKb of data, looks like you send very large files",
            offset / 1024.);
        offset = 0;
      }

      count = read(in_sock, buf + offset, sizeof(buf) - offset - 1);
      if (count < 0 && errno == EAGAIN) {
        continue;
      } else if (count < 0 && errno != EAGAIN) {
        LOG_ERROR("read error: %s", strerror(errno));
        break;
      } else if (count == 0) {
        LOG_WARNING("eof from connection with port: %d",
                    ntohs(in_addr.sin_port));
        break;
      }

      LOG_DEBUG("readen: %.1fKb", count / 1024.);

      offset += count;
      buf[offset] = '\0';

      if (buf[offset - 1] != DELIMITER) { // need more info
        LOG_DEBUG("readen not enough data");

        char *found = strrchr(buf, DELIMITER);
        if (found) {
          LOG_DEBUG("ignore old data: %.1fKb",
                    std::distance(buf, found) / 1024.);
          strcpy(buf, found);
          offset = strlen(buf);
        }

        continue;
      }


      // get latest data
      buf[offset - 1] = '\0'; // need for ignore latest delimiter
      char *start     = strrchr(buf, DELIMITER);
      start           = start ? start : buf;

      LOG_DEBUG_IF(buf != start,
                   "ignore old data: %.1fKb",
                   std::distance(buf, start) / 1024.);


      // process
      std::string response = process(start, flag_count, default_flags);
      count                = write(in_sock, response.c_str(), response.size());
      if (count < 0) {
        LOG_ERROR("failure during writting response: %s", strerror(errno));
      } else {
        LOG_DEBUG("written: %.1fKb", count / 1024.);
      }

      char delim = DELIMITER;
      count      = write(in_sock, &delim, 1);
      if (count < 0) {
        LOG_ERROR("failure during writing delimiter: %s", strerror(errno));
      }


      // clear buf
      offset = 0;
    }

    // close input sock
    LOG_INFO("close connection from port: %d", ntohs(in_addr.sin_port));
    close(in_sock);

    // just for convention
    if (default_flags) {
      delete[] default_flags;
    }
    arg_parser_dispose(parser);
    return EXIT_SUCCESS;
  }


  // finish
  signal(SIGINT, SIG_DFL);
  signal(SIGTERM, SIG_DFL);

  LOG_DEBUG("try close all children");
  for (pid_t child : children) {
    kill(child, SIGTERM);
  }
  while (true) {
    result = wait(NULL);
    if (result < 0 && errno == ECHILD) {
      LOG_DEBUG("no more children");
      break;
    } else if (result < 0) {
      LOG_ERROR("error during waiting child: %s", strerror(errno));
    } else {
      LOG_INFO("child closed: %d", result);
    }
  }

  if (default_flags) {
    delete[] default_flags;
  }

  close(sock);
  arg_parser_dispose(parser);

  LOG_INFO("finish");
  LOGGER_SHUTDOWN();
  return EXIT_SUCCESS;

Failure:
  if (sock >= 0) {
    close(sock);
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
  std::cout << c_version << std::endl;

  if (err) {
    free(err);
  }
  arg_parser_dispose(parser);
  LOGGER_SHUTDOWN();
  return EXIT_SUCCESS;

NeedHelp:
  char *usage = arg_parser_usage(parser);
  std::cout << usage;
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

  if (buf_type != "cpp" && buf_type != "c") {
    LOG_WARNING("not supported buffer type: %s", buf_type.c_str());

    jresponse[1][RETURN_CODE_TAG]   = 1;
    jresponse[1][ERROR_MESSAGE_TAG] = "unsupported buffer type: " + buf_type;
    goto Finish;
  }


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
    LOG_ERROR("error from tokenizer: %s", err.c_str());

    jresponse[1][RETURN_CODE_TAG]   = 4;
    jresponse[1][ERROR_MESSAGE_TAG] = "error from tokenizer: " + err;
    goto Finish;
  }

  for (const hl::token &token : tokens) {
    jresponse[1][TOKENS_TAG][token.group].emplace_back(token.pos);
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
