package main

import (
	"C"
	"bufio"
	"container/list"
	"encoding/json"
	"fmt"
	"go/ast"
	"go/importer"
	"go/parser"
	"go/token"
	"go/types"
	"io/ioutil"
	"log"
	"os"
)

type TokenType int

const (
	Unknown TokenType = iota
	FunctionDecl
	CallExpr
	TypeRef
	EnumConstant
  LabelRef
)

func (t TokenType) String() string {
	switch t {
	case FunctionDecl:
		return "FunctionDecl"
	case CallExpr:
		return "CallExpr"
	case TypeRef:
		return "TypeRef"
	case EnumConstant:
		return "EnumConstant"
  case LabelRef:
    return "LabelRef"
	default:
		return "Unknown"
	}
}

type Token struct {
	group TokenType
	pos   []int
}

type AstVisitor struct {
	fset   *token.FileSet
	tokens *list.List
	info   *map[*ast.Ident]types.Object
}

func (visitor AstVisitor) Visit(node ast.Node) ast.Visitor {
	if node == nil {
		return nil
	}

	var group TokenType
	var pos token.Position
	var end token.Position

	group = Unknown

	switch x := node.(type) {
  case *ast.LabeledStmt:
    group = LabelRef
    pos = (*visitor.fset).Position(x.Pos())
    end = (*visitor.fset).Position(x.Colon)
  case *ast.BranchStmt:
    if x.Label != nil {
      group = LabelRef
      pos = (*visitor.fset).Position(x.Label.Pos())
      end = (*visitor.fset).Position(x.Label.End())
    }
	case *ast.FuncDecl:
		group = FunctionDecl
		pos = (*visitor.fset).Position(x.Name.Pos())
		end = (*visitor.fset).Position(x.Name.End())
	case *ast.CallExpr:
		switch foo := x.Fun.(type) {
		case *ast.SelectorExpr:
			group = CallExpr
			pos = (*visitor.fset).Position(foo.Sel.Pos())
			end = (*visitor.fset).Position(foo.Sel.End())
		case *ast.Ident:
			if foo.Obj != nil && foo.Obj.Kind == ast.Fun {
				group = CallExpr
				pos = (*visitor.fset).Position(foo.Pos())
				end = (*visitor.fset).Position(foo.End())
			}
		}
	case *ast.Ident:
		if x.Obj != nil {
			switch x.Obj.Kind {
			case ast.Typ:
				group = TypeRef
				pos = (*visitor.fset).Position(x.Pos())
				end = (*visitor.fset).Position(x.End())
			case ast.Con:
				group = EnumConstant
				pos = (*visitor.fset).Position(x.Pos())
				end = (*visitor.fset).Position(x.End())
			}
		} else {
			if (*visitor.info)[x] != nil {
				_, is_basic := (*visitor.info)[x].Type().(*types.Basic)
				switch (*visitor.info)[x].(type) {
				case *types.Const:
					if is_basic == false {
						group = EnumConstant
						pos = (*visitor.fset).Position(x.Pos())
						end = (*visitor.fset).Position(x.End())
					}
				case *types.TypeName:
					if is_basic == false {
						group = TypeRef
						pos = (*visitor.fset).Position(x.Pos())
						end = (*visitor.fset).Position(x.End())
					}
				}
			}
		}
	}

	if group != Unknown {
		visitor.tokens.PushBack(Token{group, []int{pos.Line, pos.Column, end.Offset - pos.Offset}})
	}

	return visitor
}

func tokenize(filename string, src string) (*list.List, error) {
	var fset token.FileSet
	f, err := parser.ParseFile(&fset, filename, src, 0)
	if f == nil {
		return nil, err
	}

	info := types.Info{
		Uses: make(map[*ast.Ident]types.Object),
	}
	conf := types.Config{Importer: importer.Default()}

	pkg, err_pkg := conf.Check(f.Name.Name, &fset, []*ast.File{f}, &info)
	if pkg == nil {
		return nil, err_pkg
	}

	var visitor AstVisitor
	visitor.tokens = list.New()
	visitor.fset = &fset
	visitor.info = &info.Uses

	ast.Walk(visitor, f)

	return visitor.tokens, err
}

//export go_tokenize
func go_tokenize(filename *C.char, src *C.char, json_out **C.char, c_err **C.char) int64 {
	tokens, err := tokenize(C.GoString(filename), C.GoString(src))
	if tokens == nil {
		*c_err = C.CString(err.Error())
		return -1
	}

	obj := make(map[string][][]int)
	for el := tokens.Front(); el != nil; el = el.Next() {
		val := el.Value.(Token)
		group_name := val.group.String()
		obj[group_name] = append(obj[group_name], val.pos)
	}

	str, err_json := json.Marshal(obj)
	if str == nil {
		*c_err = C.CString(err_json.Error())
		return -1
	}

	*json_out = C.CString(string(str))
	if err != nil {
		*c_err = C.CString(err.Error())
	}

	return 0
}

func main() {
	reader := bufio.NewReader(os.Stdin)
	src, err := ioutil.ReadAll(reader)
	if src == nil {
		log.Fatal(err)
	}

	tokens, err := tokenize("blah.go", string(src))
	if tokens == nil {
		log.Fatal(err)
	} else if err != nil {
		log.Print(err)
	}

	obj := make(map[string][][]int)
	for el := tokens.Front(); el != nil; el = el.Next() {
		val := el.Value.(Token)
		group_name := val.group.String()
		obj[group_name] = append(obj[group_name], val.pos)
	}

	str, err := json.Marshal(obj)
	if str == nil {
		log.Fatal(err)
	}

	fmt.Println(string(str)) // {"bar":2,"baz":3,"foo":1}
}
