let g:hl_server_addr = "localhost:9173"

let g:hl_supported_types = ["cpp", "c"]

let g:hl_group_to_hi_link = {
      \ "Namespace"                           : "Namespace",
      \ "NamespaceAlias"                      : "Namespace",
      \ "NamespaceRef"                        : "Namespace",
      \ 
      \ "StructDecl"                          : "Type",
      \ "UnionDecl"                           : "Type",
      \ "ClassDecl"                           : "Type",
      \ "EnumDecl"                            : "Type",
      \ "TypedefDecl"                         : "Type",
      \ "TemplateTypeParameter"               : "Type",
      \ "ClassTemplate"                       : "Type",
      \ "ClassTemplatePartialSpecialization"  : "Type",
      \ "TemplateTemplateParameter"           : "Type",
      \ "UsingDirective"                      : "Type",
      \ "UsingDeclaration"                    : "Type",
      \ "TypeAliasDecl"                       : "Type",
      \ "CXXBaseSpecifier"                    : "Type",
      \ "TemplateRef"                         : "Type",
      \ "TypeRef"                             : "Type",
      \ 
      \ "Function"                            : "Function",
      \ "FunctionDecl"                        : "Function",
      \ "OverloadedDeclRef"                   : "Function",
      \ "CallExpr"                            : "Function",
      \ "Constructor"                         : "Function",
      \ "Destructor"                          : "Function",
      \ "ConversionFunction"                  : "Function",
      \ "FunctionTemplate"                    : "Function",
      \ 
      \ "Member"                              : "Member",
      \ "FieldDecl"                           : "Member",
      \ "CXXMethod"                           : "Member",
      \ "MemberRefExpr"                       : "Member",
      \ "MemberRef"                           : "Member",
      \ 
      \ "EnumConstant"                        : "EnumConstant",
      \ "EnumConstantDecl"                    : "EnumConstant",
      \ 
      \ "Variable"                            : "Variable",
      \ "VarDecl"                             : "Variable",
      \ "ParmDecl"                            : "Variable",
      \ "VariableRef"                         : "Variable",
      \ "NonTypeTemplateParameter"            : "Variable",
      \ "DeclRefExpr"                         : "Variable",
      \ 
      \ "MacroDefinition"                     : "Macro",
      \ "MacroInstantiation"                  : "Macro",
      \ "macro expansion"                     : "Macro",
      \ 
      \ "InvalidFile"                         : "Error",
      \ "NoDeclFound"                         : "Error",
      \ "InvalidCode"                         : "Error",
      \ 
      \ "CXXAccessSpecifier"                  : "Label",
      \ "LabelRef"                            : "Label",
      \ 
      \ "LinkageSpec"                         : "Normal",
      \ "FirstInvalid"                        : "Normal",
      \ "NotImplemented"                      : "Normal",
      \ "FirstExpr"                           : "Normal",
      \ "BlockExpr"                           : "Normal",
      \ 
      \ "ObjCMessageExpr"                     : "Normal",
      \ "ObjCSuperClassRef"                   : "Normal",
      \ "ObjCProtocolRef"                     : "Normal",
      \ "ObjCClassRef"                        : "Normal",
      \ "ObjCDynamicDecl"                     : "Normal",
      \ "ObjCSynthesizeDecl"                  : "Normal",
      \ "ObjCInterfaceDecl"                   : "Normal",
      \ "ObjCCategoryDecl"                    : "Normal",
      \ "ObjCProtocolDecl"                    : "Normal",
      \ "ObjCPropertyDecl"                    : "Normal",
      \ "ObjCIvarDecl"                        : "Normal",
      \ "ObjCInstanceMethodDecl"              : "Member",
      \ "ObjCClassMethodDecl"                 : "Member",
      \ "ObjCImplementationDecl"              : "Normal",
      \ "ObjCCategoryImplDecl"                : "Normal",
      \}

func EchoCallback(ch, msg)
  echo a:msg
endfunc

func MissedMsgCallback(ch, msg)
  echo "missed message"
endfunc

func ClearWinMatches(win_id)
  if exists("w:matches")
    for l:match in w:matches
      call matchdelete(l:match, a:win_id)
    endfor
  endif
  let w:matches = []
endfunc

func HighlightCallback(ch, msg)
  " check that request was processed properly
  if a:msg.return_code != 0
    echo a:msg.error_message

    " XXX even if we get fail, try highlight existed tokens
  endif

  let l:win_id = a:msg.id

  " at first clear all matches
  call ClearWinMatches(l:win_id)

  " and add new heighligth
  for l:item in a:msg.tokens
    let l:group = l:item.group
    let l:pos   = l:item.pos

    if has_key(g:hl_group_to_hi_link, l:group) == 1
      let l:hi_link = g:hl_group_to_hi_link[l:group]
      let l:match = matchaddpos(l:hi_link, [l:pos], 0, -1, {"window": l:win_id})
      if l:match != -1 " otherwise invalid match
        call add(w:matches, l:match)
      endif
    endif
  endfor
endfunc

" return flags for current buffer as list
func GetCompilationFlags()
  let l:config_file = findfile(".color_coded", ".;")
  if empty(l:config_file) == 0
    let l:flags = readfile(l:config_file)
    call add(l:flags, "-I" . expand("%:p:h")) " also add current dir as include path

    return l:flags
  end

  return []
endfunc

func SendHLRequest()
  let l:buf_type = &filetype
  if count(g:hl_supported_types, l:buf_type) != 0
    let l:buf_body = join(getline(1, "$"), "\n")

    let l:compile_flags = GetCompilationFlags()

    let l:request = {} 
    let l:request["id"] =         win_getid()
    let l:request["buf_type"] =   l:buf_type
    let l:request["buf_name"] =   buffer_name("%")
    let l:request["buf_body"] =   l:buf_body
    let l:request["additional_info"] = join(l:compile_flags, "\n")

    call ch_sendexpr(g:hl_server_channel, l:request, {"callback": "HighlightCallback"})
  endif
endfunc

call ch_logfile("log")
let g:hl_server_channel = ch_open(g:hl_server_addr, {"mode": "json", "callback": "MissedMsgCallback"})


" Vim global plugin for semantic highlighting using libclang
" Maintainer: Jeaye <contact@jeaye.com>

" LightStell color
hi default Member cterm=NONE ctermfg=147
hi default Variable cterm=NONE ctermfg=white
hi default EnumConstant cterm=NONE ctermfg=DarkGreen
hi default Namespace cterm=bold ctermfg=46

hi link StructDecl Type
hi link UnionDecl Type
hi link ClassDecl Type
hi link EnumDecl Type
hi link FieldDecl Member
hi link EnumConstantDecl EnumConstant
hi link FunctionDecl Function
hi link VarDecl Variable
hi link ParmDecl Variable
hi link ObjCInterfaceDecl Normal
hi link ObjCCategoryDecl Normal
hi link ObjCProtocolDecl Normal
hi link ObjCPropertyDecl Normal
hi link ObjCIvarDecl Normal
hi link ObjCInstanceMethodDecl Member
hi link ObjCClassMethodDecl Member
hi link ObjCImplementationDecl Normal
hi link ObjCCategoryImplDecl Normal
hi link TypedefDecl Type
hi link CXXMethod Member
hi link Namespace Namespace
hi link LinkageSpec Normal
hi link Constructor Function
hi link Destructor Function
hi link ConversionFunction Function
hi link TemplateTypeParameter Type
hi link NonTypeTemplateParameter Variable
hi link TemplateTemplateParameter Type
hi link FunctionTemplate Function
hi link ClassTemplate Type
hi link ClassTemplatePartialSpecialization Type
hi link NamespaceAlias Namespace
hi link UsingDirective Type
hi link UsingDeclaration Type
hi link TypeAliasDecl Type
hi link ObjCSynthesizeDecl Normal
hi link ObjCDynamicDecl Normal
hi link CXXAccessSpecifier Label
hi link ObjCSuperClassRef Normal
hi link ObjCProtocolRef Normal
hi link ObjCClassRef Normal
hi link TypeRef Type
hi link CXXBaseSpecifier Type
hi link TemplateRef Type
hi link NamespaceRef Namespace
hi link MemberRef Member
hi link LabelRef Label
hi link OverloadedDeclRef Function
hi link VariableRef Variable
hi link FirstInvalid Normal
hi link InvalidFile Error
hi link NoDeclFound Error
hi link NotImplemented Normal
hi link InvalidCode Error
hi link FirstExpr Normal
hi link DeclRefExpr Variable
hi link MemberRefExpr Member
hi link CallExpr Function
hi link ObjCMessageExpr Normal
hi link BlockExpr Normal
hi link MacroDefinition Macro
hi link MacroInstantiation Macro
hi link IntegerLiteral Number
hi link FloatingLiteral Float
hi link ImaginaryLiteral Number
hi link StringLiteral String
hi link CharacterLiteral Character
hi link Punctuation Normal
