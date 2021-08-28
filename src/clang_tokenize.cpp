#include "clang_tokenize.hpp"
#include <clang-c/Index.h>
#include <vector>

static const char *clang_errorToString(CXErrorCode code) noexcept;

static std::string        get_token_group(const CXCursor &cursor) noexcept;
static hl::token_location get_token_location(CXTranslationUnit translation_unit,
                                             CXToken           token) noexcept;
static std::string        map_token_kind(const CXCursorKind cursor_kind,
                                         const CXTypeKind   type_kind) noexcept;
static std::string        map_type_kind(CXTypeKind const type_kind) noexcept;

namespace hl {
hl::token_list clang_tokenize(const char * filename,
                              int          argc,
                              const char * argv[],
                              std::string &err) noexcept {
  hl::token_list        retval;
  CXIndex               index;
  CXTranslationUnit     translation_unit;
  CXErrorCode           error_code;
  CXFile                tru_file;
  size_t                file_offset;
  CXSourceLocation      begin_loc;
  CXSourceLocation      end_loc;
  CXSourceRange         range;
  CXToken *             cx_tokens = nullptr;
  unsigned int          num_tokens;
  std::vector<CXCursor> cursors;

  index = clang_createIndex(0, 0);
  error_code =
      clang_parseTranslationUnit2(index,
                                  filename,
                                  argv,
                                  argc,
                                  nullptr,
                                  0,
                                  CXTranslationUnit_DetailedPreprocessingRecord,
                                  &translation_unit);

  if (error_code != CXError_Success) {
    err = clang_errorToString(error_code);
    goto Finish;
  }

  for (uint i = 0; i < clang_getNumDiagnostics(translation_unit); ++i) {
    CXDiagnostic diag = clang_getDiagnostic(translation_unit, i);

    switch (clang_getDiagnosticSeverity(diag)) {
    case CXDiagnostic_Fatal: {
      CXString spelling =
          clang_formatDiagnostic(diag, CXDiagnostic_DisplayCategoryName);
      err = clang_getCString(spelling);
      clang_disposeString(spelling);

      goto Finish;
    } break;
    default:
      break;
    }

    clang_disposeDiagnostic(diag);
  }

  tru_file = clang_getFile(translation_unit, filename);
  if (tru_file == nullptr) {
    err = "can't get handling file from translation unit";
  }

  clang_getFileContents(translation_unit, tru_file, &file_offset);

  begin_loc = clang_getLocationForOffset(translation_unit, tru_file, 0);
  end_loc = clang_getLocationForOffset(translation_unit, tru_file, file_offset);

  range = clang_getRange(begin_loc, end_loc);


  // tokenization
  ::clang_tokenize(translation_unit, range, &cx_tokens, &num_tokens);

  if (cx_tokens == nullptr) {
    err = "no tokens";
    goto Finish;
  }


  // get annotated tokens
  cursors.resize(num_tokens);
  clang_annotateTokens(translation_unit, cx_tokens, num_tokens, cursors.data());

  for (size_t i = 0; i < num_tokens; ++i) {
    CXToken &cx_token = cx_tokens[i];

    // handle only identifiers
    if (clang_getTokenKind(cx_token) != CXToken_Identifier) {
      continue;
    }

    CXCursor &     cursor   = cursors[i];
    std::string    group    = get_token_group(cursor);
    token_location location = get_token_location(translation_unit, cx_token);
    retval.emplace_back(token{group, location});
  }


Finish:
  if (cx_tokens) {
    clang_disposeTokens(translation_unit, cx_tokens, num_tokens);
  }
  clang_disposeIndex(index);
  clang_disposeTranslationUnit(translation_unit);

  return retval;
}
} // namespace hl


static const char *clang_errorToString(CXErrorCode code) noexcept {
  switch (code) {
  case CXError_Failure:
    return "Failure";
  case CXError_Crashed:
    return "Crashed";
  case CXError_InvalidArguments:
    return "InvalidArgument";
  case CXError_ASTReadError:
    return "ASTReadError";
  case CXError_Success:
    return "Success";
  }

  return "Unknown";
}


static std::string get_token_group(const CXCursor &cursor) noexcept {
  CXTypeKind   type_kind   = clang_getCursorType(cursor).kind;
  CXCursorKind cursor_kind = clang_getCursorKind(cursor);

  return map_token_kind(cursor_kind, type_kind);
}

static hl::token_location get_token_location(CXTranslationUnit translation_unit,
                                             CXToken           token) noexcept {
  CXSourceRange    token_range = clang_getTokenExtent(translation_unit, token);
  CXSourceLocation begin       = clang_getRangeStart(token_range);
  CXSourceLocation end         = clang_getRangeEnd(token_range);

  unsigned int line;
  unsigned int column;
  unsigned int beginOffset;
  unsigned int endOffset;
  clang_getFileLocation(begin, nullptr, &line, &column, &beginOffset);
  clang_getFileLocation(end, nullptr, nullptr, nullptr, &endOffset);

  return hl::token_location{line, column, endOffset - beginOffset};
}

static std::string map_token_kind(const CXCursorKind cursor_kind,
                                  const CXTypeKind   type_kind) noexcept {
  switch (cursor_kind) {
  case CXCursor_DeclRefExpr:
  case CXCursor_VarDecl:
    return map_type_kind(type_kind);
  default:
    break;
  }

  CXString    cursorKindSpelling = clang_getCursorKindSpelling(cursor_kind);
  std::string retval             = clang_getCString(cursorKindSpelling);
  clang_disposeString(cursorKindSpelling);
  return retval;
}

static std::string map_type_kind(CXTypeKind const type_kind) noexcept {
  switch (type_kind) {
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

  CXString    typeSpelling = clang_getTypeKindSpelling(type_kind);
  std::string retval       = clang_getCString(typeSpelling);
  clang_disposeString(typeSpelling);
  return retval;
}
