// CppTokenizer.cpp

#include "CppTokenizer.hpp"
#include "gen/default_cpp_flags.hpp"
#include "logs.hpp"
#include <algorithm>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <clang-c/Index.h>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <list>
#include <regex>
#include <sstream>
#include <vector>

namespace hl::tokenizer {
namespace uuids  = boost::uuids;
using StringList = std::list<std::string>;

template <typename T>
class Finalizer {
public:
  ~Finalizer() {
    callback();
  }

  T callback;
};

template <typename T>
Finalizer<T> make_finalizer(T callback) {
  return Finalizer<T>{callback};
}

/**\brief create temporary file for compilation and write in it all buffer data
 * \throw exception if can not create file
 */
[[nodiscard]] static std::filesystem::path
getCompileFile(const std::string &bufType, const std::string &buffer);

/**\warning first value is depending from second, so be careful with using it
 */
[[nodiscard]] static std::pair<std::vector<const char *>, StringList>
getCompileFlags(const std::string &compileFlags) noexcept;

static std::string getDiagnostics(const CXTranslationUnit trUnit) noexcept;

static std::string mapTokenKind(CXCursorKind const kind,
                                CXTypeKind const   type) noexcept;

static TokenLocation getTokenLocation(CXTranslationUnit trUnit,
                                      CXToken           token) noexcept;

static std::string getTokenGroup(CXCursor cursor) noexcept;

static std::string clangErrorToString(CXErrorCode code) noexcept;

TokenList CppTokenizer::tokenize(const std::string &bufType,
                                 const std::string &buffer,
                                 const std::string &compileFlags) {
  // clang diagnostic works only with files, so we need temporary file
  std::filesystem::path compileFile = getCompileFile(bufType, buffer);
  auto                  fileRemover = make_finalizer([compileFile]() {
    std::filesystem::remove(compileFile);
  });

  auto [flags, flagList] = getCompileFlags(compileFlags);

  // clang analizing
  CXIndex           index = clang_createIndex(0, 0);
  CXTranslationUnit translationUnit;
  CXErrorCode       parseError =
      clang_parseTranslationUnit2(index,
                                  compileFile.c_str(),
                                  flags.data(),
                                  flags.size(),
                                  nullptr,
                                  0,
                                  CXTranslationUnit_DetailedPreprocessingRecord,
                                  &translationUnit);

  auto indexFinalizer  = make_finalizer([index]() {
    clang_disposeIndex(index);
  });
  auto trUnitFinalizer = make_finalizer([translationUnit]() {
    clang_disposeTranslationUnit(translationUnit);
  });

  if (parseError != CXError_Success) {
    LOG_THROW(std::runtime_error,
              "can't parse translation unit: %1%",
              clangErrorToString(parseError));
  } else if (std::string diagnostics = getDiagnostics(translationUnit);
             diagnostics.empty() == false) {
    LOG_THROW(std::runtime_error,
              "translation unit diagnostics error: %1%",
              diagnostics);
  }

  CXFile trUFile = clang_getFile(translationUnit, compileFile.c_str());
  if (trUFile == nullptr) {
    LOG_THROW(std::runtime_error,
              "can not get handling file from translation unit");
  }

  size_t endOffset = 0;
  clang_getFileContents(translationUnit, trUFile, &endOffset);

  CXSourceLocation beginLoc =
      clang_getLocationForOffset(translationUnit, trUFile, 0);
  CXSourceLocation endLoc =
      clang_getLocationForOffset(translationUnit, trUFile, endOffset);

  CXSourceRange range = clang_getRange(beginLoc, endLoc);

  unsigned int numTokens;
  CXToken *    cxTokens = nullptr;
  clang_tokenize(translationUnit, range, &cxTokens, &numTokens);

  if (cxTokens == nullptr) {
    LOG_THROW(std::runtime_error, "no tokens");
  }

  std::vector<CXCursor> cursors(numTokens);
  clang_annotateTokens(translationUnit, cxTokens, numTokens, cursors.data());

  TokenList tokens;
  for (unsigned int i = 0; i < numTokens; ++i) {
    CXToken &cxToken = cxTokens[i];

    // handle only identifiers
    if (clang_getTokenKind(cxToken) != CXToken_Identifier) {
      continue;
    }
    CXCursor &cursor = cursors[i];

    std::string   group    = getTokenGroup(cursor);
    TokenLocation location = getTokenLocation(translationUnit, cxToken);

    tokens.emplace_back(Token{group, location});
  }

  clang_disposeTokens(translationUnit, cxTokens, numTokens);

  return tokens;
}

[[nodiscard]] std::filesystem::path getCompileFile(const std::string &bufType,
                                                   const std::string &buffer) {
  std::filesystem::path tmpDir = std::filesystem::temp_directory_path();

  uuids::random_generator_mt19937 uuidGenerator;
  uuids::uuid                     uuid = uuidGenerator();

  std::string           bufferExtension = bufType;
  std::filesystem::path tmpFileName = uuids::to_string(uuid) + bufferExtension;

  std::filesystem::path tmpFile = tmpDir / tmpFileName;
  LOG_DEBUG("use temporary file: %1%", tmpFile);

  // write buffer to file
  {
    std::ofstream fout{tmpFile, std::ios::out};
    if (fout.is_open() == false) {
      std::filesystem::remove(tmpFile);
      LOG_THROW(std::runtime_error,
                "can not create temporary file for cpp buffer");
    }
    std::copy(buffer.begin(),
              buffer.end(),
              std::ostreambuf_iterator<char>{fout});
    if (fout.fail()) {
      std::filesystem::remove(tmpFile);
      LOG_THROW(std::runtime_error, "write error to temporary file");
    }
  }

  return tmpFile;
}

[[nodiscard]] std::pair<std::vector<const char *>, StringList>
getCompileFlags(const std::string &compileFlags) noexcept {
  std::vector<const char *> flags;
  StringList                flagList;
  { // at first set defaults flags
    size_t numDefaultFlags = sizeof(defaultFlags) / sizeof(defaultFlags[0]);
    for (size_t i = 0; i < numDefaultFlags; ++i) {
      flags.emplace_back(defaultFlags[i]);
    }
    LOG_DEBUG("capacity of default flags: %1%", numDefaultFlags);
  }
  if (compileFlags.empty() == false) {
    const std::regex flagsReg{R"(\s)"};
    for (auto flagIter = std::sregex_token_iterator{compileFlags.begin(),
                                                    compileFlags.end(),
                                                    flagsReg,
                                                    -1};
         flagIter != std::sregex_token_iterator{};
         ++flagIter) {
      std::string flag = flagIter->str();

      if (flag.empty()) {
        continue;
      }

      flagList.emplace_back(flag);
      flags.emplace_back(flagList.back().data());
    }
    LOG_DEBUG("capacity of manual flags: %1%", flagList.size());
  }

  return std::make_pair(std::move(flags), std::move(flagList));
}

static std::string getDiagnostics(const CXTranslationUnit trUnit) noexcept {
  StringList diagnostics;

  size_t diagnosticNum = clang_getNumDiagnostics(trUnit);
  for (size_t num = 0; num < diagnosticNum; ++num) {
    CXDiagnostic diag = clang_getDiagnostic(trUnit, num);

    switch (clang_getDiagnosticSeverity(diag)) {
    case CXDiagnostic_Fatal: {
      CXString spelling =
          clang_formatDiagnostic(diag, CXDiagnostic_DisplayCategoryName);

      diagnostics.emplace_back(clang_getCString(spelling));

      clang_disposeString(spelling);
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
            std::ostream_iterator<std::string>(ss, " "));

  return ss.str();
}

static std::string mapTypeKind(CXTypeKind const typeKind) noexcept {
  switch (typeKind) {
  case CXType_Void:
  case CXType_Bool:
  case CXType_Char_U:
  case CXType_UChar:
  case CXType_Char16:
  case CXType_Char32:
  case CXType_UShort:
  case CXType_UInt:
  case CXType_ULong:
  case CXType_ULongLong:
  case CXType_UInt128:
  case CXType_Char_S:
  case CXType_SChar:
  case CXType_WChar:
  case CXType_Short:
  case CXType_Int:
  case CXType_Long:
  case CXType_LongLong:
  case CXType_Int128:
  case CXType_Float:
  case CXType_Double:
  case CXType_LongDouble:
  case CXType_NullPtr:
  case CXType_Overload:
  case CXType_Dependent:
  case CXType_ObjCId:
  case CXType_ObjCClass:
  case CXType_ObjCSel:
  case CXType_Float128:
  case CXType_Half:
  case CXType_Float16:
  case CXType_ShortAccum:
  case CXType_Accum:
  case CXType_LongAccum:
  case CXType_UShortAccum:
  case CXType_UAccum:
  case CXType_ULongAccum:

  case CXType_Complex:
  case CXType_Pointer:
  case CXType_BlockPointer:
  case CXType_LValueReference:
  case CXType_RValueReference:
  case CXType_Record:
  case CXType_Typedef:
  case CXType_ObjCInterface:
  case CXType_ObjCObjectPointer:
  case CXType_ConstantArray:
  case CXType_Vector:
  case CXType_IncompleteArray:
  case CXType_VariableArray:
  case CXType_DependentSizedArray:
  case CXType_Auto:
  case CXType_Elaborated:
    return "Variable";

  case CXType_MemberPointer:
    return "Member";

  case CXType_Enum:
    return "EnumConstant";

  case CXType_FunctionNoProto:
  case CXType_FunctionProto:
    return "Function";

  default:
    break;
  }

  CXString    typeSpelling = clang_getTypeKindSpelling(typeKind);
  std::string retval       = clang_getCString(typeSpelling);
  clang_disposeString(typeSpelling);
  return retval;
}

static std::string mapTokenKind(CXCursorKind const cursorKind,
                                CXTypeKind const   typeKind) noexcept {
  switch (cursorKind) {
  case CXCursor_DeclRefExpr:
  case CXCursor_VarDecl:
    return mapTypeKind(typeKind);
  default:
    break;
  }

  CXString    cursorKindSpelling = clang_getCursorKindSpelling(cursorKind);
  std::string retval             = clang_getCString(cursorKindSpelling);
  clang_disposeString(cursorKindSpelling);
  return retval;
}

TokenLocation getTokenLocation(CXTranslationUnit trUnit,
                               CXToken           token) noexcept {
  CXSourceRange    tokenRange = clang_getTokenExtent(trUnit, token);
  CXSourceLocation begin      = clang_getRangeStart(tokenRange);
  CXSourceLocation end        = clang_getRangeEnd(tokenRange);

  unsigned int line;
  unsigned int column;
  unsigned int beginOffset;
  unsigned int endOffset;
  clang_getFileLocation(begin, nullptr, &line, &column, &beginOffset);
  clang_getFileLocation(end, nullptr, nullptr, nullptr, &endOffset);

  return TokenLocation{line, column, endOffset - beginOffset};
}

std::string getTokenGroup(CXCursor cursor) noexcept {
  CXTypeKind   typeKind   = clang_getCursorType(cursor).kind;
  CXCursorKind cursorKind = clang_getCursorKind(cursor);

  std::string group = mapTokenKind(cursorKind, typeKind);
  return group;
}

std::string clangErrorToString(CXErrorCode code) noexcept {
  std::string errorString;
  switch (code) {
  case CXError_Failure:
    errorString = "Failure";
    break;
  case CXError_Crashed:
    errorString = "Crashed";
    break;
  case CXError_InvalidArguments:
    errorString = "InvalidArgument";
    break;
  case CXError_ASTReadError:
    errorString = "ASTReadError";
    break;
  case CXError_Success:
    errorString = "Success";
    break;
  }

  return errorString;
}

} // namespace hl::tokenizer
