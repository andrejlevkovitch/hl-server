// CppTokenizer.cpp

#include "CppTokenizer.hpp"
#include "logs.hpp"
#include <algorithm>
#include <boost/format.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <clang-c/Index.h>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <list>
#include <memory>
#include <regex>
#include <sstream>
#include <vector>

namespace hl::tokenizer {
namespace uuids = boost::uuids;

static std::string getDiagnostics(const CXTranslationUnit trUnit);

std::string CppTokenizer::tokenize(const std::string &bufName,
                                   const std::string &buffer,
                                   const std::string &compileFlags,
                                   OUTPUT TokenList &tokens) noexcept {
  // clang diagnostic works only with files, so we need temporary file
  std::filesystem::path tmpDir = std::filesystem::temp_directory_path();

  uuids::random_generator_mt19937 uuidGenerator;
  uuids::uuid                     uuid = uuidGenerator();

  std::string bufferExtension = std::filesystem::path{bufName}.extension();
  std::filesystem::path tmpFileName = uuids::to_string(uuid) + bufferExtension;

  std::filesystem::path tmpFile = tmpDir / tmpFileName;
  LOG_DEBUG("use temporary file: %1%", tmpFile);

  // write buffer to file
  {
    std::ofstream fout{tmpFile, std::ios::out};
    if (fout.is_open() == false) {
      return "can not create file for processing";
    }
    std::copy(buffer.begin(),
              buffer.end(),
              std::ostreambuf_iterator<char>{fout});
    if (fout.fail()) {
      return "can not create file for processing";
    }
  }

  // get compile flags
  std::vector<const char *> flags;
  if (compileFlags.empty() == false) {
    std::regex reg{"\n"};
    for (auto iter = std::sregex_token_iterator{compileFlags.begin(),
                                                compileFlags.end(),
                                                reg,
                                                -1};
         iter != std::sregex_token_iterator{};
         ++iter) {
      auto beg = iter->first;
      flags.push_back(beg.base());
    }
  }

  // clang analizing
  std::string status;

  CXIndex           index = clang_createIndex(0, 0);
  CXTranslationUnit translationUnit =
      clang_parseTranslationUnit(index,
                                 tmpFile.c_str(),
                                 flags.data(),
                                 flags.size(),
                                 nullptr, // TODO what about PCH?
                                 0,
                                 CXTranslationUnit_None);

  std::string diagnostics = getDiagnostics(translationUnit);
  if (diagnostics.empty()) {
    // TODO if we get some error before, do we need lexical processing?
    // CXFile trUFile = clang_getFile(translationUnit, tmpFile.c_str());

    // XXX
  } else {
    status = diagnostics;
  }

  clang_disposeTranslationUnit(translationUnit);
  clang_disposeIndex(index);
  std::filesystem::remove(tmpFile);

  return status;
}

static std::string getDiagnostics(const CXTranslationUnit trUnit) {
  using format     = boost::format;
  using StringList = std::list<std::string>;

  StringList diagnostics;

  size_t diagnosticNum = clang_getNumDiagnostics(trUnit);
  for (size_t num = 0; num < diagnosticNum; ++num) {
    CXDiagnostic diag = clang_getDiagnostic(trUnit, num);

    switch (clang_getDiagnosticSeverity(diag)) {
    case CXDiagnostic_Error:
    case CXDiagnostic_Fatal: {
      CXString err      = clang_getDiagnosticSpelling(diag);
      CXString category = clang_getDiagnosticCategoryText(diag);

      format fmt{"Category: %1% Error: %2%"};
      fmt % clang_getCString(category) % clang_getCString(err);
      diagnostics.emplace_back(fmt.str());

      clang_disposeString(category);
      clang_disposeString(err);
    } break;
    default:
      break;
    }

    clang_disposeDiagnostic(diag);
  }

  if (diagnostics.empty()) {
    return "";
  }

  std::stringstream ss;
  std::copy(diagnostics.begin(),
            diagnostics.end(),
            std::ostream_iterator<std::string>(ss, "\n"));

  return ss.str();
}
} // namespace hl::tokenizer
