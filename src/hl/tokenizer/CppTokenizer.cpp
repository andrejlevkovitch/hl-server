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
#include <memory>
#include <regex>
#include <sstream>
#include <vector>

namespace hl::tokenizer {
namespace uuids  = boost::uuids;
using StringList = std::list<std::string>;

static std::string getDiagnostics(const CXTranslationUnit trUnit);
static std::string map_cursor_kind(CXCursorKind const kind,
                                   CXTypeKind const   type);

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
  StringList                flagList;
  { // at first set defaults flags
    size_t numDefaultFlags = sizeof(defaultFlags) / sizeof(defaultFlags[0]);
    for (size_t i = 0; i < numDefaultFlags; ++i) {
      flags.emplace_back(defaultFlags[i]);
    }
    LOG_DEBUG("capacity of default flags: %1%", numDefaultFlags);
  }
  if (compileFlags.empty() == false) {
    std::regex reg{R"(\s)"};
    for (auto iter = std::sregex_token_iterator{compileFlags.begin(),
                                                compileFlags.end(),
                                                reg,
                                                -1};
         iter != std::sregex_token_iterator{};
         ++iter) {
      std::string flag = iter->str();

      if (flag.empty()) {
        continue;
      }

      flagList.emplace_back(flag);
      flags.emplace_back(flagList.back().data());

      LOG_DEBUG("flag: %1%", flag);
    }
    LOG_DEBUG("capacity of manual flags: %1%", flagList.size());
  }

  // clang analizing
  std::string status;

  CXIndex           index = clang_createIndex(0, 0);
  CXTranslationUnit translationUnit;
  CXErrorCode       parseError =
      clang_parseTranslationUnit2(index,
                                  tmpFile.c_str(),
                                  flags.data(),
                                  flags.size(),
                                  nullptr,
                                  0,
                                  CXTranslationUnit_DetailedPreprocessingRecord,
                                  &translationUnit);

  if (parseError != CXError_Success) {
    status = "can not parse translation unit: " + std::to_string(parseError);
    goto Finish;
  } else if (std::string diagnostics = getDiagnostics(translationUnit);
             diagnostics.empty() == false) {
    status = diagnostics;
    goto Finish;
  }

  { // all right
    CXFile trUFile = clang_getFile(translationUnit, tmpFile.c_str());
    if (trUFile == nullptr) {
      status = "can not get handling file from translation unit";
      goto Finish;
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
      status = "no tokens";
      goto Finish;
    }

    std::vector<CXCursor> cursors(numTokens);
    clang_annotateTokens(translationUnit, cxTokens, numTokens, cursors.data());

    for (unsigned int i = 0; i < numTokens; ++i) {
      CXToken &cxToken = cxTokens[i];

      // handle only identifiers
      if (clang_getTokenKind(cxToken) != CXToken_Identifier) {
        continue;
      }

      CXCursor &       cursor         = cursors[i];
      CXSourceLocation cursorLocation = clang_getCursorLocation(cursor);

      unsigned int row = 0;
      unsigned int col = 0;
      clang_getFileLocation(cursorLocation, nullptr, &row, &col, nullptr);
      CXString     cursorSpelling = clang_getCursorSpelling(cursor);
      unsigned int size = std::strlen(clang_getCString(cursorSpelling));

      CXTypeKind   typeKind   = clang_getCursorType(cursor).kind;
      CXCursorKind cursorKind = clang_getCursorKind(cursor);

      std::string group = map_cursor_kind(cursorKind, typeKind);

      if (group.empty() == false) {
        tokens.emplace_back(Token{group, {row, col, size}});
      }

      clang_disposeString(cursorSpelling);
    }

    clang_disposeTokens(translationUnit, cxTokens, numTokens);
  }

Finish:
  clang_disposeTranslationUnit(translationUnit);
  clang_disposeIndex(index);
  std::filesystem::remove(tmpFile);

  return status;
}

static std::string getDiagnostics(const CXTranslationUnit trUnit) {
  StringList diagnostics;

  size_t diagnosticNum = clang_getNumDiagnostics(trUnit);
  for (size_t num = 0; num < diagnosticNum; ++num) {
    CXDiagnostic diag = clang_getDiagnostic(trUnit, num);

    switch (clang_getDiagnosticSeverity(diag)) {
    case CXDiagnostic_Error:
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
            std::ostream_iterator<std::string>(ss, "\n"));

  return ss.str();
}

static std::string map_type_kind(CXTypeKind const typeKind) {
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
    return "Variable";

  case CXType_MemberPointer:
    return "Member";

  case CXType_Enum:
    return "EnumConstant";

  case CXType_FunctionNoProto:
  case CXType_FunctionProto:
    return "Function";

  case CXType_Unexposed:
  default:
    return "";
  }
}

static std::string map_cursor_kind(CXCursorKind const cursorKind,
                                   CXTypeKind const   typeKind) {
  if (cursorKind == CXCursor_DeclRefExpr) {
    return map_type_kind(typeKind);
  }

  CXString    cursorKindSpelling = clang_getCursorKindSpelling(cursorKind);
  std::string retval             = clang_getCString(cursorKindSpelling);
  clang_disposeString(cursorKindSpelling);
  return retval;
}
} // namespace hl::tokenizer
