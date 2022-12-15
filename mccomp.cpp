#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <map>
#include <memory>
#include <queue>
#include <string.h>
#include <string>
#include <system_error>
#include <utility>
#include <vector>
using namespace llvm;
using namespace llvm::sys;

FILE *pFile;

//===----------------------------------------------------------------------===//
// Lexer
//===----------------------------------------------------------------------===//

// The lexer returns one of these for known things.
enum TOKEN_TYPE
{

  IDENT = -1,        // [a-zA-Z_][a-zA-Z_0-9]*
  ASSIGN = int('='), // '='

  // delimiters
  LBRA = int('{'),  // left brace
  RBRA = int('}'),  // right brace
  LPAR = int('('),  // left parenthesis
  RPAR = int(')'),  // right parenthesis
  SC = int(';'),    // semicolon
  COMMA = int(','), // comma

  // types
  INT_TOK = -2,   // "int"
  VOID_TOK = -3,  // "void"
  FLOAT_TOK = -4, // "float"
  BOOL_TOK = -5,  // "bool"

  // keywords
  EXTERN = -6,  // "extern"
  IF = -7,      // "if"
  ELSE = -8,    // "else"
  WHILE = -9,   // "while"
  RETURN = -10, // "return"
  // TRUE   = -12,     // "true"
  // FALSE   = -13,     // "false"

  // literals
  INT_LIT = -14,   // [0-9]+
  FLOAT_LIT = -15, // [0-9]+.[0-9]+
  BOOL_LIT = -16,  // "true" or "false" key words

  // logical operators
  AND = -17, // "&&"
  OR = -18,  // "||"

  // operators
  PLUS = int('+'),    // addition or unary plus
  MINUS = int('-'),   // substraction or unary negative
  ASTERIX = int('*'), // multiplication
  DIV = int('/'),     // division
  MOD = int('%'),     // modular
  NOT = int('!'),     // unary negation

  // comparison operators
  EQ = -19,      // equal
  NE = -20,      // not equal
  LE = -21,      // less than or equal to
  LT = int('<'), // less than
  GE = -23,      // greater than or equal to
  GT = int('>'), // greater than

  // special tokens
  EOF_TOK = 0, // signal end of file

  // invalid
  INVALID = -100 // signal invalid token
};

// TOKEN struct is used to keep track of information about a token
struct TOKEN
{
  int type = -100;
  std::string lexeme;
  int lineNo;
  int columnNo;
};

static std::string IdentifierStr; // Filled in if IDENT
static int IntVal;                // Filled in if INT_LIT
static bool BoolVal;              // Filled in if BOOL_LIT
static float FloatVal;            // Filled in if FLOAT_LIT
static std::string StringVal;     // Filled in if String Literal
static int lineNo, columnNo;

static TOKEN returnTok(std::string lexVal, int tok_type)
{
  TOKEN return_tok;
  return_tok.lexeme = lexVal;
  return_tok.type = tok_type;
  return_tok.lineNo = lineNo;
  return_tok.columnNo = columnNo - lexVal.length() - 1;
  return return_tok;
}

// Read file line by line -- or look for \n and if found add 1 to line number
// and reset column number to 0
/// gettok - Return the next token from standard input.
static TOKEN gettok()
{

  static int LastChar = ' ';
  static int NextChar = ' ';

  // Skip any whitespace.
  while (isspace(LastChar))
  {
    if (LastChar == '\n' || LastChar == '\r')
    {
      lineNo++;
      columnNo = 1;
    }
    LastChar = getc(pFile);
    columnNo++;
  }

  if (isalpha(LastChar) ||
      (LastChar == '_'))
  { // identifier: [a-zA-Z_][a-zA-Z_0-9]*
    IdentifierStr = LastChar;
    columnNo++;

    while (isalnum((LastChar = getc(pFile))) || (LastChar == '_'))
    {
      IdentifierStr += LastChar;
      columnNo++;
    }

    if (IdentifierStr == "int")
      return returnTok("int", INT_TOK);
    if (IdentifierStr == "bool")
      return returnTok("bool", BOOL_TOK);
    if (IdentifierStr == "float")
      return returnTok("float", FLOAT_TOK);
    if (IdentifierStr == "void")
      return returnTok("void", VOID_TOK);
    if (IdentifierStr == "bool")
      return returnTok("bool", BOOL_TOK);
    if (IdentifierStr == "extern")
      return returnTok("extern", EXTERN);
    if (IdentifierStr == "if")
      return returnTok("if", IF);
    if (IdentifierStr == "else")
      return returnTok("else", ELSE);
    if (IdentifierStr == "while")
      return returnTok("while", WHILE);
    if (IdentifierStr == "return")
      return returnTok("return", RETURN);
    if (IdentifierStr == "true")
    {
      BoolVal = true;
      return returnTok("true", BOOL_LIT);
    }
    if (IdentifierStr == "false")
    {
      BoolVal = false;
      return returnTok("false", BOOL_LIT);
    }

    return returnTok(IdentifierStr.c_str(), IDENT);
  }

  if (LastChar == '=')
  {
    NextChar = getc(pFile);
    if (NextChar == '=')
    { // EQ: ==
      LastChar = getc(pFile);
      columnNo += 2;
      return returnTok("==", EQ);
    }
    else
    {
      LastChar = NextChar;
      columnNo++;
      return returnTok("=", ASSIGN);
    }
  }

  if (LastChar == '{')
  {
    LastChar = getc(pFile);
    columnNo++;
    return returnTok("{", LBRA);
  }
  if (LastChar == '}')
  {
    LastChar = getc(pFile);
    columnNo++;
    return returnTok("}", RBRA);
  }
  if (LastChar == '(')
  {
    LastChar = getc(pFile);
    columnNo++;
    return returnTok("(", LPAR);
  }
  if (LastChar == ')')
  {
    LastChar = getc(pFile);
    columnNo++;
    return returnTok(")", RPAR);
  }
  if (LastChar == ';')
  {
    LastChar = getc(pFile);
    columnNo++;
    return returnTok(";", SC);
  }
  if (LastChar == ',')
  {
    LastChar = getc(pFile);
    columnNo++;
    return returnTok(",", COMMA);
  }

  if (isdigit(LastChar) || LastChar == '.')
  { // Number: [0-9]+.
    std::string NumStr;

    if (LastChar == '.')
    { // Floatingpoint Number: .[0-9]+
      do
      {
        NumStr += LastChar;
        LastChar = getc(pFile);
        columnNo++;
      } while (isdigit(LastChar));

      FloatVal = strtof(NumStr.c_str(), nullptr);
      return returnTok(NumStr, FLOAT_LIT);
    }
    else
    {
      do
      { // Start of Number: [0-9]+
        NumStr += LastChar;
        LastChar = getc(pFile);
        columnNo++;
      } while (isdigit(LastChar));

      if (LastChar == '.')
      { // Floatingpoint Number: [0-9]+.[0-9]+)
        do
        {
          NumStr += LastChar;
          LastChar = getc(pFile);
          columnNo++;
        } while (isdigit(LastChar));

        FloatVal = strtof(NumStr.c_str(), nullptr);
        return returnTok(NumStr, FLOAT_LIT);
      }
      else
      { // Integer : [0-9]+
        IntVal = strtod(NumStr.c_str(), nullptr);
        return returnTok(NumStr, INT_LIT);
      }
    }
  }

  if (LastChar == '&')
  {
    NextChar = getc(pFile);
    if (NextChar == '&')
    { // AND: &&
      LastChar = getc(pFile);
      columnNo += 2;
      return returnTok("&&", AND);
    }
    else
    {
      LastChar = NextChar;
      columnNo++;
      return returnTok("&", int('&'));
    }
  }

  if (LastChar == '|')
  {
    NextChar = getc(pFile);
    if (NextChar == '|')
    { // OR: ||
      LastChar = getc(pFile);
      columnNo += 2;
      return returnTok("||", OR);
    }
    else
    {
      LastChar = NextChar;
      columnNo++;
      return returnTok("|", int('|'));
    }
  }

  if (LastChar == '!')
  {
    NextChar = getc(pFile);
    if (NextChar == '=')
    { // NE: !=
      LastChar = getc(pFile);
      columnNo += 2;
      return returnTok("!=", NE);
    }
    else
    {
      LastChar = NextChar;
      columnNo++;
      return returnTok("!", NOT);
      ;
    }
  }

  if (LastChar == '<')
  {
    NextChar = getc(pFile);
    if (NextChar == '=')
    { // LE: <=
      LastChar = getc(pFile);
      columnNo += 2;
      return returnTok("<=", LE);
    }
    else
    {
      LastChar = NextChar;
      columnNo++;
      return returnTok("<", LT);
    }
  }

  if (LastChar == '>')
  {
    NextChar = getc(pFile);
    if (NextChar == '=')
    { // GE: >=
      LastChar = getc(pFile);
      columnNo += 2;
      return returnTok(">=", GE);
    }
    else
    {
      LastChar = NextChar;
      columnNo++;
      return returnTok(">", GT);
    }
  }

  if (LastChar == '/')
  { // could be division or could be the start of a comment
    LastChar = getc(pFile);
    columnNo++;
    if (LastChar == '/')
    { // definitely a comment
      do
      {
        LastChar = getc(pFile);
        columnNo++;
      } while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

      if (LastChar != EOF)
        return gettok();
    }
    else
      return returnTok("/", DIV);
  }

  // Check for end of file.  Don't eat the EOF.
  if (LastChar == EOF)
  {
    columnNo++;
    return returnTok("0", EOF_TOK);
  }

  // Otherwise, just return the character as its ascii value.
  int ThisChar = LastChar;
  std::string s(1, ThisChar);
  LastChar = getc(pFile);
  columnNo++;
  return returnTok(s, int(ThisChar));
}

//===----------------------------------------------------------------------===//
// Parsertok_identifierd updates CurTok with its results.
static TOKEN CurTok;
static std::deque<TOKEN> tok_buffer;
static TOKEN error_token; // for error recovery in the syntax analysis
static int errorLineNo, errorColumnNo;

static TOKEN getNextToken()
{
  error_token = CurTok;
  errorLineNo = CurTok.lineNo;
  errorColumnNo = CurTok.columnNo;

  if (tok_buffer.size() == 0)
    tok_buffer.push_back(gettok());

  TOKEN temp = tok_buffer.front();

  tok_buffer.pop_front();

  return CurTok = temp;
}

// Return the first token from the buffer without removing it.
static TOKEN lookahead1()
{
  if (tok_buffer.size() == 0)
    tok_buffer.push_back(gettok());
  return tok_buffer.front();
}

// Return the second token from the buffer without removing it.
static TOKEN lookahead2()
{
  if (tok_buffer.size() == 0)
    tok_buffer.push_back(gettok());

  if (tok_buffer.size() == 1)
    tok_buffer.push_back(gettok());

  return tok_buffer[1];
}

static void putBackToken(TOKEN tok) { tok_buffer.push_front(tok); }

//===----------------------------------------------------------------------===//
// AST nodes
//===----------------------------------------------------------------------===//

/// ASTnode - Base class for all AST nodes.
class ASTnode
{
public:
  virtual ~ASTnode() {}
  virtual Value *codegen() = 0;
  virtual std::string to_string() const
  {
    return "ASTnode";
  };
};

/// LogError* - These are little helper functions for error handling.
std::unique_ptr<ASTnode> LogError(const char *Str)
{
  fprintf(stderr, "Ln: %d, Col:%d - Syntax Error: %s\n", errorLineNo, errorColumnNo, Str);
  exit(-1);
  return nullptr;
}

std::unique_ptr<ASTnode> LogErrorSemantic(const char *Str, TOKEN tok)
{
  fprintf(stderr, "Ln: %d, Col:%d - Semantic Error: %s\n", tok.lineNo, tok.columnNo, Str);
  exit(-1);
  return nullptr;
}

std::string LogErrorStr(const char *Str)
{
  fprintf(stderr, "Ln: %d, Col:%d - Syntax Error: %s\n", errorLineNo, errorColumnNo, Str);
  exit(-1);
  return "";
}

/// IntASTnode - Class for integer literals like 1, 2, 10,
class IntASTnode : public ASTnode
{
  TOKEN Tok; // token of the integer literal
  int Val;

public:
  IntASTnode(TOKEN tok, int val) : Val(val), Tok(tok) {}

  virtual std::string to_string() const override
  {
    return std::to_string(Val);
  };

  Value *codegen() override;
};

// FloatASTnode - Class for floating point literals like 1.0, 2.0, 10.0
class FloatASTnode : public ASTnode
{
  float Val;
  TOKEN Tok; // token of the float literal

public:
  FloatASTnode(TOKEN tok, float val) : Val(val), Tok(tok) {}

  virtual std::string to_string() const override
  {
    return std::to_string(Val);
  };

  Value *codegen() override;
};

// BoolASTnode - Class for boolean literals like true, false
class BoolASTnode : public ASTnode
{
  bool Val;
  TOKEN Tok; // token of the boolean literal

public:
  BoolASTnode(TOKEN tok, bool val) : Val(val), Tok(tok) {}

  virtual std::string to_string() const override
  {
    return std::to_string(Val);
  };

  Value *codegen() override;
};

// VarCallASTnode - Class for variable calls like a, b, c
class VarCallASTnode : public ASTnode
{
  std::string Name;
  TOKEN Tok; // token of the call

public:
  VarCallASTnode(TOKEN tok, std::string name) : Name(name), Tok(tok) {}

  virtual std::string to_string() const override
  {
    return Name;
  };

  Value *codegen() override;
};

// VarDeclASTnode - Class for variable declarations and function parameters like int a, float b, bool c -
class VarDeclASTnode : public ASTnode
{
  std::string Name;
  std::string Type;
  TOKEN Tok; // token at the name of the variable declaration

public:
  VarDeclASTnode(TOKEN Tok, std::string Name, std::string Type)
      : Name(Name), Type(Type), Tok(Tok) {}

  virtual std::string to_string() const override
  {
    std::string s = "Variable Decl: " + Type + " " + Name;
    return s;
  };

  const std::string getName() const { return Name; }

  const std::string getType() const { return Type; }

  Value *codegen() override;
};

// LogErrorP - error handling for parameter nodes
std::unique_ptr<VarDeclASTnode> LogErrorP(const char *Str)
{
  fprintf(stderr, "Ln: %d, Col:%d - Syntax Error: %s\n", errorLineNo, errorColumnNo, Str);
  exit(-1);
  return nullptr;
}

// UnaryASTnode - Class for unary expressions like -1 and !false
class UnaryASTnode : public ASTnode
{
  TOKEN Tok; // token at the unary expression
  char Op;
  std::unique_ptr<ASTnode> RHS;

public:
  UnaryASTnode(TOKEN tok, char Op, std::unique_ptr<ASTnode> RHS)
      : Tok(tok), Op(Op), RHS(std::move(RHS)) {}

  virtual std::string to_string() const override
  {
    return std::string(std::to_string(Op) + RHS->to_string());
  }

  Value *codegen() override;
};

// BinaryASTnode - Class for binary expressions like + and <
class BinaryASTnode : public ASTnode
{
  TOKEN Tok; // token at the operand
  std::string Op;
  std::unique_ptr<ASTnode> LHS, RHS;

public:
  BinaryASTnode(TOKEN tok, std::unique_ptr<ASTnode> LHS, std::unique_ptr<ASTnode> RHS, std::string op)
      : Tok(tok), LHS(std::move(LHS)), RHS(std::move(RHS)), Op(op) {}

  virtual std::string to_string() const override
  {
    return std::string(LHS->to_string() + " " + Op + " " + RHS->to_string());
  };

  Value *codegen() override;
};

// FunctionCallASTnode - Class for function calls
class FunctionCallASTnode : public ASTnode
{
  TOKEN Tok;
  std::string Name;
  std::vector<std::unique_ptr<ASTnode>> Args;

public:
  FunctionCallASTnode(TOKEN tok, std::string Name, std::vector<std::unique_ptr<ASTnode>> Args)
      : Tok(tok), Name(Name), Args(std::move(Args)) {}

  virtual std::string to_string() const override
  {
    return std::string(Name + "(" + Args[0]->to_string() + ")");
  };

  Value *codegen() override;
};

// BlockASTnode - Class for blocks of code
class BlockASTnode : public ASTnode
{
  int IndentLevel;
  std::vector<std::unique_ptr<ASTnode>> local_decls;
  std::vector<std::unique_ptr<ASTnode>> statements;
  TOKEN Tok; // token at the beginning of the block

public:
  BlockASTnode(TOKEN Tok, std::vector<std::unique_ptr<ASTnode>> local_decls, std::vector<std::unique_ptr<ASTnode>> statements, int indentLevel)
      : local_decls(std::move(local_decls)), statements(std::move(statements)), IndentLevel(indentLevel), Tok(Tok) {}

  virtual std::string to_string() const override
  {
    std::string s = "";
    std::string gap = "|    ";
    std::string indent = "|____";

    for (auto &i : local_decls)
    {
      s += "\n";
      for (int j = 0; j < IndentLevel - 1; j++)
      {
        s += gap;
      }
      s += indent + i->to_string();
    }
    for (auto &i : statements)
    {
      if (i != nullptr)
      {
        s += "\n";
        for (int j = 0; j < IndentLevel - 1; j++)
        {
          s += gap;
        }
        s += indent + i->to_string();
      }
    }
    return s;
  };

  Value *codegen() override;
};

// WhileASTnode - Class for while loops
class WhileASTnode : public ASTnode
{
  std::unique_ptr<ASTnode> Condition;
  std::unique_ptr<ASTnode> Stmt;
  TOKEN Tok; // token at the while keyword

public:
  WhileASTnode(TOKEN Tok, std::unique_ptr<ASTnode> Condition, std::unique_ptr<ASTnode> Stmt)
      : Condition(std::move(Condition)), Stmt(std::move(Stmt)), Tok(Tok) {}

  virtual std::string to_string() const override
  {
    std::string s = "While: " + Condition->to_string() + " " + Stmt->to_string();
    return s;
  };

  Value *codegen() override;
};

// IfASTnode - Class for if statements
class IfASTnode : public ASTnode
{
  std::unique_ptr<ASTnode> IfCondition;
  std::unique_ptr<ASTnode> IfBlock, ElseBlock;
  int IndentLevel;
  TOKEN Tok; // token at the if keyword

public:
  IfASTnode(TOKEN Tok, std::unique_ptr<ASTnode> IfCondition, std::unique_ptr<ASTnode> IfBlock, std::unique_ptr<ASTnode> ElseBlock, int IndentLevel)
      : IfCondition(std::move(IfCondition)), IfBlock(std::move(IfBlock)), ElseBlock(std::move(ElseBlock)), IndentLevel(IndentLevel), Tok(Tok) {}

  virtual std::string to_string() const override
  {
    std::string s = "If: " + IfCondition->to_string() + " " + IfBlock->to_string();
    if (ElseBlock != nullptr)
    {
      s += "\n";
      for (int i = 0; i < IndentLevel - 1; i++)
      {
        s += "|    ";
      }
      s += "|____Else: " + ElseBlock->to_string();
    }
    return s;
  };

  Value *codegen() override;
};

// AssignASTnode - Class for assignments like x = 1
class AssignASTnode : public ASTnode
{
  std::string Name;
  std::unique_ptr<ASTnode> RHS;
  TOKEN Tok; // token at the equals sign

public:
  AssignASTnode(TOKEN Tok, std::string name, std::unique_ptr<ASTnode> RHS)
      : Name(name), RHS(std::move(RHS)), Tok(Tok) {}

  virtual std::string to_string() const override
  {
    std::string s = "Assign: " + Name + " = " + RHS->to_string();
    return s;
  };

  Value *codegen() override;
};

// PrototypeASTnode - Class for function prototypes
class PrototypeASTnode
{
  std::string Name;
  std::vector<std::unique_ptr<VarDeclASTnode>> Params;
  std::string Type_spec;
  TOKEN Tok;

public:
  PrototypeASTnode(TOKEN Tok, std::string name, std::vector<std::unique_ptr<VarDeclASTnode>> Params, std::string Type_spec)
      : Name(name), Params(std::move(Params)), Type_spec(Type_spec), Tok(Tok) {}

  virtual std::string to_string() const
  {
    std::string s = "Function Declaration: " + Name + "(";
    for (auto &param : Params)
    {
      s += param->to_string() + ", ";
    }
    // get rid of the last comma
    if (Params.size() > 0)
    {
      s = s.substr(0, s.size() - 2);
    }
    s += ") -> " + Type_spec;
    return s;
  };

  const std::string getName() const { return Name; }
  const std::vector<std::unique_ptr<VarDeclASTnode>> &getParams() const { return Params; }
  const std::string getType() const { return Type_spec; }

  Function *codegen();
};

// ExternASTnode - Class for extern declarations
class ExternASTnode : public ASTnode
{
  std::string Type;
  std::string Name;
  std::vector<std::unique_ptr<VarDeclASTnode>> Params;
  TOKEN Tok; // Token at the function name

public:
  ExternASTnode(TOKEN Tok, std::string Type, std::string Name, std::vector<std::unique_ptr<VarDeclASTnode>> Params)
      : Type(Type), Name(Name), Params(std::move(Params)), Tok(Tok) {}

  virtual std::string to_string() const override
  {
    std::string s = "Extern: " + Name + " (";
    for (auto &param : Params)
    {
      s += param->to_string() + ", ";
    }
    s = s.substr(0, s.size() - 2);
    s += ")";
    return s;
  };

  Function *codegen() override;
};

// FunDeclASTnode - Class for function definitions
class FunDeclASTnode : public ASTnode
{
  std::unique_ptr<PrototypeASTnode> Prototype;
  std::unique_ptr<ASTnode> Block;
  TOKEN Tok; // Token at the function name
public:
  FunDeclASTnode(TOKEN Tok, std::unique_ptr<PrototypeASTnode> Prototype, std::unique_ptr<ASTnode> Block)
      : Prototype(std::move(Prototype)), Block(std::move(Block)), Tok(Tok) {}

  virtual std::string to_string() const
  {
    if (Block != nullptr)
      return std::string(Prototype->to_string() + Block->to_string());
    else
      return std::string(Prototype->to_string());
  };

  Function *codegen();

  const std::string getName() const { return Prototype->getName(); }
};

// ReturnASTnode - Class for return statements
class ReturnASTnode : public ASTnode
{
  std::unique_ptr<ASTnode> ReturnExpression;
  TOKEN Tok; // token at the return keyword

public:
  ReturnASTnode(TOKEN Tok, std::unique_ptr<ASTnode> ReturnExpression)
      : ReturnExpression(std::move(ReturnExpression)), Tok(Tok) {}

  virtual std::string to_string() const override
  {
    if (ReturnExpression != nullptr)
    {
      return std::string("Return: " + ReturnExpression->to_string());
    }
    else
    {
      return std::string("Return: ");
    }
  };

  Value *codegen() override;
};

// ProgramASTnode - Root of the AST
class ProgramASTnode : public ASTnode
{
  std::vector<std::unique_ptr<ASTnode>> Extern_list;
  std::vector<std::unique_ptr<ASTnode>> Decl_list;
  TOKEN Tok; // token at the start of the program
public:
  ProgramASTnode(TOKEN Tok, std::vector<std::unique_ptr<ASTnode>> Extern_list, std::vector<std::unique_ptr<ASTnode>> Decl_list)
      : Extern_list(std::move(Extern_list)), Decl_list(std::move(Decl_list)), Tok(Tok) {}

  std::string to_string() const override
  {
    std::string s = "Program: ";
    for (auto &i : Extern_list)
    {
      s += "\n|____" + i->to_string() + " ";
    }

    for (auto &i : Decl_list)
    {
      s += "\n|____" + i->to_string() + " ";
    }

    s += "\n|EOF";
    return s;
  };

  Value *codegen() override;
};

//===----------------------------------------------------------------------===//
// Recursive Descent Parser - Function call for each production
//===----------------------------------------------------------------------===//
// Note: Parser is written bottom up (ie: the root node is the last to be declared)

// global variable for the indent level for the to_string methods
int indentLevel = 1;

static std::unique_ptr<ASTnode> ParseExpr();

// arg_list' ::= expr "," arg_list' | epsilon
static std::vector<std::unique_ptr<ASTnode>> ParseArgListPrime()
{
  // FOLLOW(arg_list') = {")"}
  if (CurTok.type == RBRA)
  {
    return std::vector<std::unique_ptr<ASTnode>>(); // empty vector == epsilon
  }
  else
  {
    std::vector<std::unique_ptr<ASTnode>> arglist;
    auto Expression = ParseExpr(); // parse the expression
    while (CurTok.type == COMMA)
    {
      getNextToken();                           // eat the ,
      arglist.push_back(std::move(Expression)); // add the expression to the vector
      Expression = ParseExpr();                 // parse the next expression
    }
    arglist.push_back(std::move(Expression)); // add the last expression to the vector
    return arglist;
  }
}

// arg_list ::= expr "," arg_list'
static std::vector<std::unique_ptr<ASTnode>> ParseArgList()
{
  auto Expression = ParseExpr(); // parse the expression
  if (CurTok.type == COMMA)
  {
    getNextToken();                                                   // eat the ,
    auto ArgListPrime = ParseArgListPrime();                          // parse the arg_list'
    ArgListPrime.insert(ArgListPrime.begin(), std::move(Expression)); // if there is an arg_list', add the expression to the front of the vector
    return ArgListPrime;
  }
  else
  { // if there is no arg_list', return a vector with just the expression
    std::vector<std::unique_ptr<ASTnode>> arglist;
    arglist.push_back(std::move(Expression));
    return arglist;
  }
}

// args ::= arg_list
// |  epsilon
static std::vector<std::unique_ptr<ASTnode>> ParseArgs()
{
  if (CurTok.type == RBRA)
  {                                                 // FOLLOW(epsilon) = {")"}
    return std::vector<std::unique_ptr<ASTnode>>(); // empty vector == epsilon
  }
  else
  {
    return ParseArgList(); // parse the arg_list
  }
}

// rval1 ::=  "-" rval1 | "!" rval1
//       | "(" expr ")"
//       | IDENT | IDENT "(" args ")"
//       | INT_LIT | FLOAT_LIT | BOOL_LIT

static std::unique_ptr<ASTnode> ParseRval1()
{
  std::unique_ptr<ASTnode> Result;
  switch (CurTok.type)
  {
  default:
    return LogError("Unknown token when expecting an expression");
  case MINUS:
  {
    TOKEN a = CurTok;
    getNextToken(); // eat the -
    return std::make_unique<UnaryASTnode>(a, '-', ParseRval1());
  }
  case NOT:
  {
    TOKEN a = CurTok;
    getNextToken(); // eat the !
    return std::make_unique<UnaryASTnode>(a, '!', ParseRval1());
  }
  case LPAR:
  {
    getNextToken();                // eat the (
    auto Expression = ParseExpr(); // eat expr
    if (CurTok.type != RPAR)
    {
      return LogError("Expected )"); // FOLLOW(expr) = )
    }
    getNextToken(); // eat the )
    return Expression;
  }
  case IDENT:
  {
    std::string identifierStr = IdentifierStr;
    TOKEN a = CurTok;
    getNextToken(); // eat the IDENT
    if (CurTok.type != LPAR)
    { // if the next token is not a (, then it is a variable
      return std::make_unique<VarCallASTnode>(a, identifierStr);
    }
    else
    {                          // if the next token is a (, then it is a function call
      getNextToken();          // eat (
      auto Args = ParseArgs(); // parse the args - if no args then it will return an empty vector
      if (CurTok.type != RPAR)
      { // FOLLOW(args) = )
        return LogError("Expected )");
      }
      getNextToken(); // eat )
      return std::make_unique<FunctionCallASTnode>(a, identifierStr, std::move(Args));
    }
  }
  case INT_LIT:
  {
    Result = std::make_unique<IntASTnode>(CurTok, IntVal);
    getNextToken(); // eat the number
    return std::move(Result);
  }
  case FLOAT_LIT:
  {
    Result = std::make_unique<FloatASTnode>(CurTok, FloatVal);
    getNextToken(); // eat the number
    return std::move(Result);
  }
  case BOOL_LIT:
  {
    Result = std::make_unique<BoolASTnode>(CurTok, BoolVal);
    getNextToken(); // eat the bool
    return std::move(Result);
  }
  }
}

// rval2' ::= "*" rval1 rval2'
// | "/" rval1 rval2'
// | "%" rval1 rval2'
// | epsilon
static std::unique_ptr<ASTnode> ParseRval2Prime(std::unique_ptr<ASTnode> LHS)
{
  if (CurTok.type == ASTERIX)
  {
    TOKEN a = CurTok;
    getNextToken();                                                                                  // eat the *
    std::unique_ptr<ASTnode> RHS = ParseRval1();                                                     // parse rval1
    return ParseRval2Prime(std::make_unique<BinaryASTnode>(a, std::move(LHS), std::move(RHS), "*")); // parse rval2'
  }
  else if (CurTok.type == DIV)
  {
    TOKEN a = CurTok;
    getNextToken();                                                                                  // eat the /
    std::unique_ptr<ASTnode> RHS = ParseRval1();                                                     // parse rval1
    return ParseRval2Prime(std::make_unique<BinaryASTnode>(a, std::move(LHS), std::move(RHS), "/")); // parse rval2'
  }
  else if (CurTok.type == MOD)
  {
    TOKEN a = CurTok;
    getNextToken();                                                                                  // eat the %
    std::unique_ptr<ASTnode> RHS = ParseRval1();                                                     // parse rval1
    return ParseRval2Prime(std::make_unique<BinaryASTnode>(a, std::move(LHS), std::move(RHS), "%")); // parse rval2'
  }
  else
  { // Pass the epsilon through
    return LHS;
  }
}

// rval2 ::= rval1 rval2'
static std::unique_ptr<ASTnode> ParseRval2()
{
  std::unique_ptr<ASTnode> LHS = ParseRval1(); // parse rval1
  return ParseRval2Prime(std::move(LHS));      // parse rval2'
}

// rval3' ::= "+" rval2 rval3'
// | "-" rval2 rval3'
// | epsilon
static std::unique_ptr<ASTnode> ParseRval3Prime(std::unique_ptr<ASTnode> LHS)
{
  if (CurTok.type == PLUS)
  {
    TOKEN a = CurTok;
    getNextToken();                                                                                  // eat the +
    std::unique_ptr<ASTnode> RHS = ParseRval2();                                                     // parse rval2
    return ParseRval3Prime(std::make_unique<BinaryASTnode>(a, std::move(LHS), std::move(RHS), "+")); // parse rval3'
  }
  else if (CurTok.type == MINUS)
  {
    TOKEN a = CurTok;
    getNextToken();                                                                                  // eat the -
    std::unique_ptr<ASTnode> RHS = ParseRval2();                                                     // parse rval2
    return ParseRval3Prime(std::make_unique<BinaryASTnode>(a, std::move(LHS), std::move(RHS), "-")); // parse rval3'
  }
  else
  {
    return LHS; // Pass the epsilon through
  }
}

//  rval3 ::= rval2 rval3'
static std::unique_ptr<ASTnode> ParseRval3()
{
  std::unique_ptr<ASTnode> LHS = ParseRval2(); // parse rval2
  return ParseRval3Prime(std::move(LHS));      // parse rval3'
}

// rval4' ::= "<=" rval3 rval4'
// | "<" rval3 rval4'
// | ">=" rval3 rval4'
// | ">" rval3 rval4'
static std::unique_ptr<ASTnode> ParseRval4Prime(std::unique_ptr<ASTnode> LHS)
{
  if (CurTok.type == LE)
  {
    TOKEN a = CurTok;
    getNextToken();                                                                                   // eat the <=
    std::unique_ptr<ASTnode> RHS = ParseRval3();                                                      // parse rval3
    return ParseRval4Prime(std::make_unique<BinaryASTnode>(a, std::move(LHS), std::move(RHS), "<=")); // parse rval4'
  }
  else if (CurTok.type == LT)
  {
    TOKEN a = CurTok;
    getNextToken();                                                                                  // eat the <
    std::unique_ptr<ASTnode> RHS = ParseRval3();                                                     // parse rval3
    return ParseRval4Prime(std::make_unique<BinaryASTnode>(a, std::move(LHS), std::move(RHS), "<")); // parse rval4'
  }
  else if (CurTok.type == GE)
  {
    TOKEN a = CurTok;
    getNextToken();                                                                                   // eat the >=
    std::unique_ptr<ASTnode> RHS = ParseRval3();                                                      // parse rval3
    return ParseRval4Prime(std::make_unique<BinaryASTnode>(a, std::move(LHS), std::move(RHS), ">=")); // parse rval4'
  }
  else if (CurTok.type == GT)
  {
    TOKEN a = CurTok;
    getNextToken();                                                                                  // eat the >
    std::unique_ptr<ASTnode> RHS = ParseRval3();                                                     // parse rval3
    return ParseRval4Prime(std::make_unique<BinaryASTnode>(a, std::move(LHS), std::move(RHS), ">")); // parse rval4'
  }
  else
  {
    return LHS;
  }
}

// rval4 ::= rval3 rval4'
static std::unique_ptr<ASTnode> ParseRval4()
{
  std::unique_ptr<ASTnode> LHS = ParseRval3(); // parse rval3
  return ParseRval4Prime(std::move(LHS));      // parse rval4'
}

// rval5' ::= "==" rval4 rval5'
// | "!=" rval4 rval5'
// | epsilon
static std::unique_ptr<ASTnode> ParseRval5Prime(std::unique_ptr<ASTnode> LHS)
{
  if (CurTok.type == EQ)
  {
    TOKEN a = CurTok;
    getNextToken();                                                                                   // eat the ==
    std::unique_ptr<ASTnode> RHS = ParseRval4();                                                      // parse rval4
    return ParseRval5Prime(std::make_unique<BinaryASTnode>(a, std::move(LHS), std::move(RHS), "==")); // parse rval5'
  }
  else if (CurTok.type == NE)
  {
    TOKEN a = CurTok;
    getNextToken();                                                                                   // eat the !=
    std::unique_ptr<ASTnode> RHS = ParseRval4();                                                      // parse rval4
    return ParseRval5Prime(std::make_unique<BinaryASTnode>(a, std::move(LHS), std::move(RHS), "!=")); // parse rval5'
  }
  else
  { // Pass the epsilon through
    return LHS;
  }
}

// rval5 ::= rval4 rval5'
static std::unique_ptr<ASTnode> ParseRval5()
{
  std::unique_ptr<ASTnode> LHS = ParseRval4(); // parse rval4
  return ParseRval5Prime(std::move(LHS));      // parse rval5'
}

// rval6' ::= "&&" rval5 rval6'
// | epsilon
static std::unique_ptr<ASTnode> ParseRval6Prime(std::unique_ptr<ASTnode> LHS)
{
  if (CurTok.type == AND)
  {
    TOKEN a = CurTok;
    getNextToken();                                                                                   // eat the &&
    std::unique_ptr<ASTnode> RHS = ParseRval5();                                                      // parse rval5
    return ParseRval6Prime(std::make_unique<BinaryASTnode>(a, std::move(LHS), std::move(RHS), "&&")); // parse rval6'
  }
  else
  { // Pass the epsilon through
    return LHS;
  }
}

// rval6 ::= rval5 rval6'
static std::unique_ptr<ASTnode> ParseRval6()
{
  std::unique_ptr<ASTnode> LHS = ParseRval5(); // parse rval5
  return ParseRval6Prime(std::move(LHS));      // parse rval6'
}

// rval7' ::= "||" rval6 rval7'
// | epsilon
static std::unique_ptr<ASTnode> ParseRval7Prime(std::unique_ptr<ASTnode> LHS)
{
  if (CurTok.type == OR)
  {
    TOKEN a = CurTok;
    getNextToken();                                                                                   // eat the ||
    std::unique_ptr<ASTnode> RHS = ParseRval6();                                                      // parse rval6
    return ParseRval7Prime(std::make_unique<BinaryASTnode>(a, std::move(LHS), std::move(RHS), "||")); // parse rval7'
  }
  else
  { // Pass the epsilon through
    return LHS;
  }
}

// rval7 ::= rval6 rval7'
static std::unique_ptr<ASTnode> ParseRval7()
{
  std::unique_ptr<ASTnode> LHS = ParseRval6(); // parse rval6
  return ParseRval7Prime(std::move(LHS));      // parse rval7'
}

// expr ::= IDENT "=" expr
// | rval7
static std::unique_ptr<ASTnode> ParseExpr()
{
  if (CurTok.type == IDENT)
  { // could be an rval or an assignment - FIRST(rval7) = { IDENT,INT_LIT,FLOAT_LIT,BOOL_LIT,"-","!","("}
    if (lookahead1().type == ASSIGN)
    {
      std::string Name = IdentifierStr;
      getNextToken(); // eat the IDENT
      TOKEN a = CurTok;
      getNextToken();                                                   // eat the =
      std::unique_ptr<ASTnode> expr = ParseExpr();                      // parse expr
      return std::make_unique<AssignASTnode>(a, Name, std::move(expr)); // return the assignment
    }
    else
    {
      return ParseRval7(); // parse rval7
    }
  }
  else
  {
    return ParseRval7(); // parse rval7
  }
}

// return_stmt ::= "return" ";"
// |  "return" expr ";"
static std::unique_ptr<ASTnode> ParseReturnStmt()
{
  if (CurTok.type == RETURN)
  {
    TOKEN a = CurTok;
    getNextToken(); // eat the return
    if (CurTok.type == SC)
    {
      getNextToken();                                     // eat the ;
      return std::make_unique<ReturnASTnode>(a, nullptr); // void return
    }
    else
    {
      std::unique_ptr<ASTnode> expr = ParseExpr(); // parse expr
      if (CurTok.type == SC)
      {
        getNextToken();                                             // eat the ;
        return std::make_unique<ReturnASTnode>(a, std::move(expr)); // return with expr
      }
      else
      {
        return LogError("Expected ;");
      }
    }
  }
  else
  {
    return LogError("Expected return statement");
  }
}

static std::unique_ptr<ASTnode> ParseBlock();

static std::unique_ptr<ASTnode> ParseStmt();

// else_stmt  ::= "else" block
// |  epsilon
static std::unique_ptr<ASTnode> ParseElseStmt()
{
  if (CurTok.type == ELSE)
  {
    getNextToken();      // eat the else
    return ParseBlock(); // parse block
  }
  // FIRST(block) = { IDENT,INT_LIT,FLOAT_LIT,BOOL_LIT,"-","!","(",LBRA,IF,WHILE,ELSE,RETURN,RBRA}
  if (CurTok.type == IDENT || CurTok.type == INT_LIT || CurTok.type == FLOAT_LIT || CurTok.type == BOOL_LIT || CurTok.type == MINUS || CurTok.type == NOT || CurTok.type == LPAR || CurTok.type == LBRA || CurTok.type == IF || CurTok.type == WHILE || CurTok.type == ELSE || CurTok.type == RETURN || CurTok.type == RBRA)
  {
    return nullptr; // epsilon transition == nullptr
  }
  else
  {
    return LogError("Expected 'else' statement or another statement");
  }
}

// if_stmt ::= "if" "(" expr ")" block else_stmt
static std::unique_ptr<ASTnode> ParseIfStmt()
{
  if (CurTok.type == IF)
  {
    TOKEN a = CurTok;
    getNextToken(); // eat the if
    if (CurTok.type == LPAR)
    {
      getNextToken();                                     // eat the (
      std::unique_ptr<ASTnode> ifCondition = ParseExpr(); // parse expr
      if (CurTok.type == RPAR)
      {
        getNextToken(); // eat the )
        indentLevel++;
        std::unique_ptr<ASTnode> ifBlock = ParseBlock();      // parse block
        std::unique_ptr<ASTnode> elseBlock = ParseElseStmt(); // parse else_stmt
        std::unique_ptr<ASTnode> ifast = std::make_unique<IfASTnode>(a, std::move(ifCondition), std::move(ifBlock), std::move(elseBlock), indentLevel);
        indentLevel--;
        return ifast;
      }
      else
      {
        return LogError("Expected )");
      }
    }
    else
    {
      return LogError("Expected (");
    }
  }
  else
  {
    return LogError("Expected 'if' keyword");
  }
}

// expr_stmt ::= expr ";"
// |  ";"

static std::unique_ptr<ASTnode> ParseExprStmt()
{
  // FIRST(expr) = { IDENT,INT_LIT,FLOAT_LIT,BOOL_LIT,"-","!","("}
  if (CurTok.type == IDENT || CurTok.type == INT_LIT || CurTok.type == FLOAT_LIT || CurTok.type == BOOL_LIT || CurTok.type == MINUS || CurTok.type == NOT || CurTok.type == LPAR)
  {
    std::unique_ptr<ASTnode> expr = ParseExpr(); // parse expr
    if (CurTok.type == SC)
    {
      getNextToken(); // eat the ;
      return expr;
    }
    else
    {
      return LogError("Expected ;");
    }
  }
  else if (CurTok.type == SC)
  {
    getNextToken(); // eat the ;
    return nullptr; // epsilon transition == nullptr
  }
  else
  {
    return LogError("Expected expression statement or ;");
  }
}

// while_stmt ::= "while" "(" expr ")" stmt
static std::unique_ptr<ASTnode> ParseWhileStmt()
{
  if (CurTok.type == WHILE)
  {
    TOKEN a = CurTok;
    getNextToken(); // eat the while
    if (CurTok.type == LPAR)
    {
      getNextToken();                              // eat the (
      std::unique_ptr<ASTnode> expr = ParseExpr(); // parse expr
      if (CurTok.type == RPAR)
      {
        getNextToken(); // eat the )
        indentLevel = indentLevel + 2;
        std::unique_ptr<ASTnode> stmt = ParseStmt(); // parse stmt
        indentLevel = indentLevel - 1;
        std::unique_ptr<WhileASTnode> w = std::make_unique<WhileASTnode>(a, std::move(expr), std::move(stmt));
        indentLevel = indentLevel - 1;
        return w;
      }
      else
      {
        return LogError("Expected )");
      }
    }
    else
    {
      return LogError("Expected (");
    }
  }
  else
  {
    return LogError("Expected 'while' keyword");
  }
}

// stmt ::= expr_stmt
// |  block
// |  if_stmt
// |  while_stmt
// |  return_stmt
static std::unique_ptr<ASTnode> ParseStmt()
{
  // FIRST(expr_stmt) = { IDENT,INT_LIT,FLOAT_LIT,BOOL_LIT,"-","!","(",";"}
  if (CurTok.type == IDENT || CurTok.type == INT_LIT || CurTok.type == FLOAT_LIT || CurTok.type == BOOL_LIT || CurTok.type == MINUS || CurTok.type == NOT || CurTok.type == LPAR || CurTok.type == SC)
  {
    return ParseExprStmt();
  }
  // FIRST(block)={"{"}
  else if (CurTok.type == LBRA)
  {
    return ParseBlock();
  }
  // FIRST(if_stmt)={"if"}
  else if (CurTok.type == IF)
  {
    return ParseIfStmt();
  }
  // FIRST(while_stmt) = {"while"}
  else if (CurTok.type == WHILE)
  {
    return ParseWhileStmt();
  }
  // FIRST(return_stmt) = {"return"}
  else if (CurTok.type == RETURN)
  {
    return ParseReturnStmt();
  }
  else
  {
    return LogError("Expected expression statement, block, if statement, while statement, or return statement");
  }
}

// stmt_list ::= stmt stmt_list
// |  epsilon
static std::vector<std::unique_ptr<ASTnode>> ParseStmtList()
{
  std::vector<std::unique_ptr<ASTnode>> stmt_list;
  // FIRST(stmt) = { IDENT,INT_LIT,FLOAT_LIT,BOOL_LIT,"-","!","(","{","if","while","return",";"}
  while (CurTok.type == IDENT || CurTok.type == INT_LIT || CurTok.type == FLOAT_LIT || CurTok.type == BOOL_LIT || CurTok.type == MINUS || CurTok.type == NOT || CurTok.type == LPAR || CurTok.type == LBRA || CurTok.type == SC || CurTok.type == IF || CurTok.type == WHILE || CurTok.type == ELSE || CurTok.type == RETURN)
  {
    stmt_list.push_back(ParseStmt()); // parse stmt
  }
  // FOLLOW(stmt_list) = {"}"}
  if (CurTok.type == RBRA)
  {
    return stmt_list;
  }
  else
  {
    std::vector<std::unique_ptr<ASTnode>> errors;
    std::unique_ptr<ASTnode> error = LogError("Expected }");
    errors.push_back(std::move(error));
    return errors;
  }
}

// not necessary but makes the code more readable
//  var_type  ::= "int" |  "float" | "bool"
static std::string ParseVarType()
{
  if (CurTok.type == INT_TOK)
  {
    return "int";
  }
  else if (CurTok.type == FLOAT_TOK)
  {
    return "float";
  }
  else if (CurTok.type == BOOL_TOK)
  {
    return "bool";
  }
  else
  {
    return LogErrorStr("Expected variable type - 'int', 'float', or 'bool'");
  }
}

// local_decl ::= var_type IDENT ";"
static std::unique_ptr<ASTnode> ParseLocalDecl()
{
  // FIRST(var_type) = {"int","float","bool"}
  if (CurTok.type == INT_TOK || CurTok.type == FLOAT_TOK || CurTok.type == BOOL_TOK)
  {
    std::string var_type = ParseVarType(); // parse var_type
    getNextToken();                        // eat the var_type
    if (CurTok.type == IDENT)
    {
      TOKEN a = CurTok;
      std::string IDENT = IdentifierStr; // get the IDENT name
      getNextToken();                    // eat the IDENT
      if (CurTok.type == SC)
      {
        getNextToken(); // eat the ;
        return std::make_unique<VarDeclASTnode>(a, IDENT, var_type);
      }
      else
      {
        return LogError("Expected ;");
      }
    }
    else
    {
      return LogError("Expected variable name");
    }
  }
  else
  {
    return LogError("Expected variable type - 'int', 'float', or 'bool'");
  }
}

// local_decls ::= local_decl local_decls
// |  epsilon
static std::vector<std::unique_ptr<ASTnode>> ParseLocalDecls()
{
  std::vector<std::unique_ptr<ASTnode>> local_decls; // return empty vector if it is an epsilon transition
  // FIRST(local_decl) = {"int","float","bool"}
  while (CurTok.type == INT_TOK || CurTok.type == FLOAT_TOK || CurTok.type == BOOL_TOK)
  {
    local_decls.push_back(ParseLocalDecl()); // parse local_decl
  }
  // FOLLOW(local_decls) = {IDENT,INT_LIT,FLOAT_LIT,BOOL_LIT,"-","!","(",";","{","if","while","else","return","}"}
  if (CurTok.type == IDENT || CurTok.type == INT_LIT || CurTok.type == FLOAT_LIT || CurTok.type == BOOL_LIT || CurTok.type == MINUS || CurTok.type == NOT || CurTok.type == LPAR || CurTok.type == LBRA || CurTok.type == IF || CurTok.type == WHILE || CurTok.type == ELSE || CurTok.type == RETURN || CurTok.type == RBRA)
  {
    return local_decls;
  }
  else
  {
    std::vector<std::unique_ptr<ASTnode>> errors;
    std::unique_ptr<ASTnode> error = LogError("Expected variable name, expression statement, 'if', 'while', 'else', 'return', or }");
    errors.push_back(std::move(error));
    return errors;
  }
}

// block ::= "{" local_decls stmt_list "}"
static std::unique_ptr<ASTnode> ParseBlock()
{
  if (CurTok.type == LBRA)
  {
    getNextToken(); // eat the {
    TOKEN a = CurTok;
    std::vector<std::unique_ptr<ASTnode>> local_decls = ParseLocalDecls(); // parse local_decls
    std::vector<std::unique_ptr<ASTnode>> stmt_list = ParseStmtList();     // parse stmt_list
    if (CurTok.type == RBRA)
    {
      getNextToken(); // eat the }
      indentLevel++;
      std::unique_ptr<ASTnode> block = std::make_unique<BlockASTnode>(a, std::move(local_decls), std::move(stmt_list), indentLevel);
      indentLevel--;
      return block;
    }
    else
    {
      return LogError("Expected }");
    }
  }
  else
  {
    return LogError("Expected {");
  }
}

// param ::= var_type IDENT
static std::unique_ptr<VarDeclASTnode> ParseParam()
{
  if (CurTok.type == INT_TOK || CurTok.type == FLOAT_TOK || CurTok.type == BOOL_TOK)
  {
    std::string var_type = ParseVarType(); // parse var_type
    getNextToken();
    if (CurTok.type == IDENT)
    {
      TOKEN a = CurTok;
      std::string ident = IdentifierStr; // get the IDENT name
      getNextToken();                    // eat the IDENT
      return std::make_unique<VarDeclASTnode>(a, ident, var_type);
    }
    else
    {
      return LogErrorP("Expected variable name");
    }
  }
  else
  {
    return LogErrorP("Expected variable type - 'int', 'float', or 'bool'");
  }
}

// param_list' ::= param "," paramlist' | epsilon
static std::vector<std::unique_ptr<VarDeclASTnode>> ParseParamListPrime()
{
  std::vector<std::unique_ptr<VarDeclASTnode>> param_list; // return empty vector if it is an epsilon transition
  std::unique_ptr<VarDeclASTnode> param = ParseParam();    // parse param - returns nullptr if it is an epsilon transition
  while (CurTok.type == COMMA)
  {
    getNextToken();                     // eat the ,
    param_list.push_back(ParseParam()); // parse param
  }
  if (CurTok.type != RPAR)
  {
    std::vector<std::unique_ptr<VarDeclASTnode>> errors;
    std::unique_ptr<VarDeclASTnode> error = LogErrorP("Expected )");
    errors.push_back(std::move(error));
    return errors;
  }
  else
  {
    param_list.insert(std::move(param_list.begin()), std::move(param));
    return param_list;
  }
}

// param_list ::= param "," param_list'
//                | param
static std::vector<std::unique_ptr<VarDeclASTnode>> ParseParamList()
{
  if (CurTok.type == INT_TOK || CurTok.type == FLOAT_TOK || CurTok.type == BOOL_TOK)
  {
    std::unique_ptr<VarDeclASTnode> param = ParseParam(); // parse param
    if (CurTok.type == COMMA)
    {                                                                                  // param_list' transition
      getNextToken();                                                                  // eat the ,
      std::vector<std::unique_ptr<VarDeclASTnode>> param_list = ParseParamListPrime(); // parse param_list'
      if (param)
      {
        param_list.insert(param_list.begin(), std::move(param)); // insert param into the front of the vector
      }
      return param_list;
    }
    else
    { // param_list ::= param
      std::vector<std::unique_ptr<VarDeclASTnode>> param_list;
      param_list.push_back(std::move(param));
      return param_list;
    }
  }
  else
  { // param_list ::= epsilon
    std::unique_ptr<VarDeclASTnode> error = LogErrorP("Expected variable type - 'int', 'float', or 'bool'");
    std::vector<std::unique_ptr<VarDeclASTnode>> param_list;
    param_list.push_back(std::move(error));
    return param_list;
  }
}

// params ::= param_list
// |  "void" | epsilon
static std::vector<std::unique_ptr<VarDeclASTnode>> ParseParams()
{
  if (CurTok.type == INT_TOK || CurTok.type == FLOAT_TOK || CurTok.type == BOOL_TOK)
  {
    return ParseParamList(); // parse param_list
  }
  else if (CurTok.type == VOID_TOK)
  {
    std::unique_ptr<VarDeclASTnode> v = std::make_unique<VarDeclASTnode>(CurTok, "", "void"); // create a void variable
    getNextToken();                                                                           // eat the void
    std::vector<std::unique_ptr<VarDeclASTnode>> param_list;
    param_list.push_back(std::move(v));
    return param_list;
  }
  else if (CurTok.type == RPAR)
  { // FOLLOW(params) = {")"}
    return std::vector<std::unique_ptr<VarDeclASTnode>>{};
  }
  else
  {
    std::unique_ptr<VarDeclASTnode> error = LogErrorP("Incorrect parameter declaration - expected parameter type, 'void' or ')'");
    std::vector<std::unique_ptr<VarDeclASTnode>> errors;
    errors.push_back(std::move(error));
    return errors;
  }
}

// type_spec ::= "void"
// |  var_type
static std::string ParseTypeSpec()
{
  if (CurTok.type == VOID_TOK)
  {
    return "void"; // return void
  }
  else if (CurTok.type == INT_TOK || CurTok.type == FLOAT_TOK || CurTok.type == BOOL_TOK)
  {
    return ParseVarType(); // parse var_type
  }
  else
  {
    return LogErrorStr("Expected type specifier - 'int', 'float', 'bool', or 'void'");
  }
}

// fun_decl ::= type_spec IDENT "(" params ")" block
static std::unique_ptr<ASTnode> ParseFunDecl()
{
  if (CurTok.type == INT_TOK || CurTok.type == FLOAT_TOK || CurTok.type == VOID_TOK || CurTok.type == BOOL_TOK)
  {
    std::string type_spec = ParseTypeSpec(); // parse type_spec
    getNextToken();                          // eat the type_spec
    if (CurTok.type == IDENT)
    {
      TOKEN a = CurTok;
      std::string name = IdentifierStr; // get the IDENT name
      getNextToken();                   // eat the IDENT
      if (CurTok.type == LPAR)
      {
        getNextToken();                                                      // eat the (
        std::vector<std::unique_ptr<VarDeclASTnode>> params = ParseParams(); // parse params
        if (CurTok.type == RPAR)
        {
          getNextToken(); // eat the )
          // Fundecl = Prototype + Block
          std::unique_ptr<ASTnode> block = ParseBlock(); // parse block
          std::unique_ptr<PrototypeASTnode> proto = std::make_unique<PrototypeASTnode>(a, name, std::move(params), type_spec);
          return std::make_unique<FunDeclASTnode>(a, std::move(proto), std::move(block));
        }
        else
        {
          return LogError("Expected )");
        }
      }
      else
      {
        return LogError("Expected (");
      }
    }
    else
    {
      return LogError("Expected function name");
    }
  }
  else
  {
    return LogError("Expected type specifier - 'int', 'float', 'bool', or 'void'");
  }
}

// var_decl ::= var_type IDENT ";"
static std::unique_ptr<ASTnode> ParseVarDecl()
{
  // FIRST(var_type) = {"int", "float", "bool"}
  if (CurTok.type == INT_TOK || CurTok.type == FLOAT_TOK || CurTok.type == BOOL_TOK)
  {
    std::string var_type = ParseVarType(); // parse var_type
    getNextToken();
    if (CurTok.type == IDENT)
    {
      std::string name = IdentifierStr; // get the IDENT name
      TOKEN a = CurTok;
      getNextToken(); // eat the IDENT
      if (CurTok.type == SC)
      {
        getNextToken(); // eat the ;
        return std::make_unique<VarDeclASTnode>(a, name, var_type);
      }
      else
      {
        return LogError("Expected ;");
      }
    }
    else
    {
      return LogError("Expected variable name");
    }
  }
  else
  {
    return LogError("Expected variable type - 'int', 'float', or 'bool'");
  }
}

// decl ::= var_decl
// |  fun_decl
static std::unique_ptr<ASTnode> ParseDecl()
{
  if (CurTok.type == INT_TOK || CurTok.type == FLOAT_TOK || CurTok.type == BOOL_TOK)
  {
    // either var_decl or fun_decl so we need to look ahead 2 to see if it is a semi-colon or bracket
    if (lookahead1().type == IDENT)
    {
      if (lookahead2().type == LPAR)
      {
        // fun_decl
        return ParseFunDecl();
      }
      else if (lookahead2().type == SC)
      {
        // var_decl
        return ParseVarDecl();
      }
      else
      {
        return LogError("Expected ; or ( for variable and function declaration respectively");
      }
    }
    else
    {
      return LogError("Expected function or variable name");
    }
  }
  else if (CurTok.type == VOID_TOK)
  { // VOID_TOK is only in FIRST(fun_decl)
    return ParseFunDecl();
  }
  else
  {
    return LogError("Expected type specifier - 'int', 'float', 'bool', or 'void'");
  }
}

// decl_list' ::= decl decl_list'
// | epsilon
static std::vector<std::unique_ptr<ASTnode>> ParseDeclListPrime()
{
  std::vector<std::unique_ptr<ASTnode>> decl_list;
  while (CurTok.type == INT_TOK || CurTok.type == FLOAT_TOK || CurTok.type == VOID_TOK || CurTok.type == BOOL_TOK)
  {
    decl_list.push_back(ParseDecl()); // parse decl
  }
  if (CurTok.type != EOF_TOK)
  {
    std::vector<std::unique_ptr<ASTnode>> errors;
    std::unique_ptr<ASTnode> error = LogError("Expected EOF");
    errors.push_back(std::move(error));
    return errors;
  }
  return decl_list;
}

// decl_list ::= decl decl_list'
static std::vector<std::unique_ptr<ASTnode>> ParseDeclList()
{
  if (CurTok.type == INT_TOK || CurTok.type == FLOAT_TOK || CurTok.type == VOID_TOK || CurTok.type == BOOL_TOK)
  {
    std::unique_ptr<ASTnode> decl = ParseDecl(); // parse decl
    if (decl)
    {
      std::vector<std::unique_ptr<ASTnode>> decl_list = ParseDeclListPrime(); // parse decl list'
      decl_list.insert(decl_list.begin(), std::move(decl));
      return decl_list;
    }
    else
    {
      std::unique_ptr<ASTnode> error = LogError("Expected function or variable declaration ");
      std::vector<std::unique_ptr<ASTnode>> errors;
      errors.push_back(std::move(error));
      return errors;
    }
  }
  else
  {
    std::unique_ptr<ASTnode> error = LogError("Expected type specifier - 'int', 'float', 'bool', or 'void'");
    std::vector<std::unique_ptr<ASTnode>> errors;
    errors.push_back(std::move(error));
    return errors;
  }
}

// extern ::= "extern" type_spec IDENT "(" params ")" ";"
// FIRST(extern) = {extern}
static std::unique_ptr<ASTnode> ParseExtern()
{
  if (CurTok.type == EXTERN)
  {
    getNextToken(); // eat the extern
    if (CurTok.type == INT_TOK || CurTok.type == FLOAT_TOK || CurTok.type == BOOL_TOK || CurTok.type == VOID_TOK)
    {
      std::string type_spec = ParseTypeSpec(); // deal with the type_spec
      getNextToken();
      if (CurTok.type == IDENT)
      {
        TOKEN a = CurTok;
        std::string IDENT = IdentifierStr; // get the IDENT name
        getNextToken();                    // eat the IDENT
        if (CurTok.type == LPAR)
        {                                                                      // deal with the params
          getNextToken();                                                      // eat the (
          std::vector<std::unique_ptr<VarDeclASTnode>> params = ParseParams(); // parse the params
          if (CurTok.type == RPAR)
          {                 // deal with the )
            getNextToken(); // eat the )
            if (CurTok.type == SC)
            {                 // deal with the ;
              getNextToken(); // eat the ;

              return std::make_unique<ExternASTnode>(a, type_spec, IDENT, std::move(params));
            }
            else
            {
              return LogError("Expected ;");
            }
          }
          else
          {
            return LogError("Expected )");
          }
        }
        else
        {
          return LogError("Expected (");
        }
      }
      else
      {
        return LogError("Expected function name");
      }
    }
    else
    {
      return LogError("Expected type specifier - 'int', 'float', 'bool', or 'void'");
    }
  }
  else
  {
    return LogError("Expected 'extern' keyword");
  }
}

// extern_list' ::= extern extern_List'
// | epsilon
static std::vector<std::unique_ptr<ASTnode>> ParseExternListPrime()
{
  std::vector<std::unique_ptr<ASTnode>> extern_list;
  // FIRST(extern) = {extern}
  while (CurTok.type == EXTERN)
  {
    extern_list.push_back(ParseExtern()); // parse extern
  }
  // FOLLOW(extern_list') = {"int","float","bool","void"}
  if (CurTok.type == INT_TOK || CurTok.type == FLOAT_TOK || CurTok.type == BOOL_TOK || CurTok.type == VOID_TOK)
  {
    return extern_list;
  }
  else
  {
    std::vector<std::unique_ptr<ASTnode>> errorlist;
    std::unique_ptr<ASTnode> error = LogError("Expected type specifier - 'int', 'float', 'bool', or 'void'");
    errorlist.push_back(std::move(error));
    return extern_list;
  }
}

// extern_list ::=  extern extern_list'
static std::vector<std::unique_ptr<ASTnode>> ParseExternList()
{
  if (CurTok.type == EXTERN)
  {                                                                             // FIRST(extern) = {extern}
    std::unique_ptr<ASTnode> extern_node = ParseExtern();                       // parse extern
    std::vector<std::unique_ptr<ASTnode>> extern_list = ParseExternListPrime(); // parse extern list'
    extern_list.insert(extern_list.begin(), std::move(extern_node));            // add extern to front of list
    return extern_list;
  }
  else
  {
    std::unique_ptr<ASTnode> error = LogError("Expected 'extern' keyword");
    std::vector<std::unique_ptr<ASTnode>> errors;
    errors.push_back(std::move(error));
    return errors;
  }
}

// program ::= extern_list decl_list
// | decl_list
static std::unique_ptr<ASTnode> ParseProgram()
{
  TOKEN a = CurTok;
  if (CurTok.type == EXTERN)
  {
    std::vector<std::unique_ptr<ASTnode>> extern_list = ParseExternList(); // parse extern list
    // FIRST(program) = {EXTERN, INT_TOK, FLOAT_TOK, BOOL_TOK, VOID_TOK}
    if (CurTok.type == INT_TOK || CurTok.type == FLOAT_TOK || CurTok.type == BOOL_TOK || CurTok.type == VOID_TOK)
    {
      std::vector<std::unique_ptr<ASTnode>> decl_list = ParseDeclList(); // parse decl list
      if (CurTok.type == EOF_TOK)
      { // FOLLOW(program) = {EOF_TOK}
        std::unique_ptr<ProgramASTnode> program = std::make_unique<ProgramASTnode>(a, std::move(extern_list), std::move(decl_list));
        return program;
      }
      else
      {
        return LogError("Expected EOF");
      }
    }
    else
    {
      return LogError("Expected type specifier - 'int', 'float', 'bool', or 'void'");
    }
  }
  else if (CurTok.type == INT_TOK || CurTok.type == FLOAT_TOK || CurTok.type == BOOL_TOK || CurTok.type == VOID_TOK)
  {
    std::vector<std::unique_ptr<ASTnode>> decl_list = ParseDeclList(); // parse decl list
    if (CurTok.type == EOF_TOK)
    { // check for eof
      std::vector<std::unique_ptr<ASTnode>> extern_list;
      std::unique_ptr<ProgramASTnode> program = std::make_unique<ProgramASTnode>(a, std::move(extern_list), std::move(decl_list));
      return program;
    }
    else
    {
      return LogError("Expected EOF");
    }
  }
  else
  {
    return LogError("Expected extern declaration or function declaration or variable declaration");
  }
}

static std::unique_ptr<ASTnode> parser()
{
  return ParseProgram();
}

//===----------------------------------------------------------------------===//
// Code Generation
//===----------------------------------------------------------------------===//

static LLVMContext TheContext;
static IRBuilder<> Builder(TheContext);
static std::unique_ptr<Module> TheModule;

static std::unique_ptr<legacy::FunctionPassManager> TheFPM;

// runtime stack of local variables
static std::map<int, std::map<std::string, AllocaInst *>> VariableStack; // local variables as a stack
static std::map<std::string, GlobalVariable *> GlobalVariables;          // global variables

// runtime level
static int level = 0;

// error message for values
Value *LogErrorV(const char *Str, TOKEN tok)
{
  LogErrorSemantic(Str, tok);
  exit(-1);
  return nullptr;
}
// error message for functions
Function *LogErrorF(const char *Str, TOKEN tok)
{
  LogErrorSemantic(Str, tok);
  exit(-1);
  return nullptr;
}

static AllocaInst *CreateEntryBlockAlloca(Function *TheFunction, StringRef VarName, Type *type)
{
  IRBuilder<> TmpB(&TheFunction->getEntryBlock(), TheFunction->getEntryBlock().begin());
  return TmpB.CreateAlloca(type, nullptr, VarName);
}

Value *IntASTnode::codegen()
{
  return ConstantInt::get(TheContext, APInt(32, Val, true));
}

Value *FloatASTnode::codegen()
{
  return ConstantFP::get(TheContext, APFloat(Val));
}

Value *BoolASTnode::codegen()
{
  if (Val)
    return ConstantInt::get(TheContext, APInt(1, 1, true));
  else
    return ConstantInt::get(TheContext, APInt(1, 0, true));
}

Value *VarCallASTnode::codegen()
{
  // check the local scope for the variable
  for (int i = level; i >= 0; i--)
  {
    if (AllocaInst *alloca = VariableStack[i][Name])
    {
      return Builder.CreateLoad(alloca->getAllocatedType(), alloca, Name.c_str());
    }
  }
  // if not found in local scope, check if a global variable exists
  if (auto *G = TheModule->getNamedGlobal(Name))
    return Builder.CreateLoad(G->getValueType(), G, Name.c_str());
  return LogErrorV("Unknown variable name called", Tok);
}

Value *VarDeclASTnode::codegen()
{
  // check if variable is already declared in the current scope
  for (int i = level; i >= 0; i--)
  {
    if (VariableStack[i][Name])
    {
      return LogErrorV("Variable already declared in the local scope", Tok);
    }
  }

  // Create type and value for the alloca
  llvm::Type *type = nullptr;
  Constant *v = nullptr;
  if (Type == "int")
  {
    type = Type::getInt32Ty(TheContext);
    v = Constant::getNullValue(Type::getInt32Ty(TheContext));
  }
  else if (Type == "float")
  {
    type = Type::getFloatTy(TheContext);
    v = Constant::getNullValue(Type::getFloatTy(TheContext));
  }
  else if (Type == "bool")
  {
    type = Type::getInt1Ty(TheContext);
    v = Constant::getNullValue(Type::getInt1Ty(TheContext));
  }
  else
  {
    return LogErrorV("Unknown type", Tok);
  }

  // Create the alloca
  if (Builder.GetInsertBlock())
  {
    // local case
    Function *TheFunction = Builder.GetInsertBlock()->getParent();
    AllocaInst *Alloca = CreateEntryBlockAlloca(TheFunction, Name, type);
    VariableStack[level][Name] = Alloca;
    return Alloca;
  }
  else
  {
    // global case
    GlobalVariable *g = new GlobalVariable(*(TheModule.get()), type, false, GlobalValue::CommonLinkage, v);
    g->setAlignment(MaybeAlign(4));
    g->setName(Name);
    return g;
  }
};

Value *UnaryASTnode::codegen()
{
  Value *R = RHS->codegen();
  if (!R)
    return nullptr;

  switch (Op)
  { // check the types and operands
  case '-':
    if (R->getType()->isIntegerTy(32))
    {
      return Builder.CreateNeg(R, "negtmp");
    }
    else if (R->getType()->isFloatingPointTy())
    {
      return Builder.CreateFNeg(R, "negtmp");
    }
    else
    {
      return LogErrorV("Unknown type", Tok);
    }
  case '!':
    if (R->getType()->isIntegerTy(1))
    {
      return Builder.CreateNot(R, "nottmp");
    }
    else
    {
      return LogErrorV("Unknown type", Tok);
    }
  default:
    return LogErrorV("Invalid unary operator", Tok);
  }
}

Value *BinaryASTnode::codegen()
{
  Value *left = LHS->codegen();
  Value *right = RHS->codegen();

  if (!left || !right)
  {
    return nullptr;
  }

  Type *leftType = left->getType();
  Type *rightType = right->getType();

  // if both sides are ints
  if (leftType == Type::getInt32Ty(TheContext) && rightType == Type::getInt32Ty(TheContext))
  {
    if (Op == "+")
    {
      return Builder.CreateAdd(left, right, "addtmp");
    }
    else if (Op == "-")
    {
      return Builder.CreateSub(left, right, "subtmp");
    }
    else if (Op == "*")
    {
      return Builder.CreateMul(left, right, "multmp");
    }
    else if (Op == "/")
    {
      return Builder.CreateSDiv(left, right, "divtmp");
    }
    else if (Op == "%")
    {
      return Builder.CreateSRem(left, right, "remtmp");
    }
    else if (Op == "<")
    {
      return Builder.CreateICmpSLT(left, right, "cmptmp");
    }
    else if (Op == ">")
    {
      return Builder.CreateICmpSGT(left, right, "cmptmp");
    }
    else if (Op == "<=")
    {
      return Builder.CreateICmpSLE(left, right, "cmptmp");
    }
    else if (Op == ">=")
    {
      return Builder.CreateICmpSGE(left, right, "cmptmp");
    }
    else if (Op == "==")
    {
      return Builder.CreateICmpEQ(left, right, "cmptmp");
    }
    else if (Op == "!=")
    {
      return Builder.CreateICmpNE(left, right, "cmptmp");
    }
    else
    {
      return LogErrorV("invalid binary operator", Tok);
    }
  } // if both sides are floats
  else if (leftType == Type::getFloatTy(TheContext) && rightType == Type::getFloatTy(TheContext))
  {
    if (Op == "+")
    {
      return Builder.CreateFAdd(left, right, "addtmp");
    }
    else if (Op == "-")
    {
      return Builder.CreateFSub(left, right, "subtmp");
    }
    else if (Op == "*")
    {
      return Builder.CreateFMul(left, right, "multmp");
    }
    else if (Op == "/")
    {
      return Builder.CreateFDiv(left, right, "divtmp");
    }
    else if (Op == "%")
    {
      return Builder.CreateFRem(left, right, "remtmp");
    }
    else if (Op == "<")
    {
      return Builder.CreateFCmpULT(left, right, "cmptmp");
    }
    else if (Op == ">")
    {
      return Builder.CreateFCmpUGT(left, right, "cmptmp");
    }
    else if (Op == "<=")
    {
      return Builder.CreateFCmpULE(left, right, "cmptmp");
    }
    else if (Op == ">=")
    {
      return Builder.CreateFCmpUGE(left, right, "cmptmp");
    }
    else if (Op == "==")
    {
      return Builder.CreateFCmpUEQ(left, right, "cmptmp");
    }
    else if (Op == "!=")
    {
      return Builder.CreateFCmpUNE(left, right, "cmptmp");
    }
    else
    {
      return LogErrorV("invalid binary operator", Tok);
    }
  } // if one side is an int and one side is a float
  else if ((leftType == Type::getInt32Ty(TheContext) && rightType == Type::getFloatTy(TheContext)) || (leftType == Type::getFloatTy(TheContext) && rightType == Type::getInt32Ty(TheContext)))
  {
    // cast the int to a float
    if (leftType == Type::getInt32Ty(TheContext))
    {
      left = Builder.CreateSIToFP(left, Type::getFloatTy(TheContext), "casttmp");
    }
    else
    {
      right = Builder.CreateSIToFP(right, Type::getFloatTy(TheContext), "casttmp");
    }

    if (Op == "+")
    {
      return Builder.CreateFAdd(left, right, "addtmp");
    }
    else if (Op == "-")
    {
      return Builder.CreateFSub(left, right, "subtmp");
    }
    else if (Op == "*")
    {
      return Builder.CreateFMul(left, right, "multmp");
    }
    else if (Op == "/")
    {
      return Builder.CreateFDiv(left, right, "divtmp");
    }
    else if (Op == "%")
    {
      return Builder.CreateFRem(left, right, "remtmp");
    }
    else if (Op == "<")
    {
      return Builder.CreateFCmpULT(left, right, "cmptmp");
    }
    else if (Op == ">")
    {
      return Builder.CreateFCmpUGT(left, right, "cmptmp");
    }
    else if (Op == "<=")
    {
      return Builder.CreateFCmpULE(left, right, "cmptmp");
    }
    else if (Op == ">=")
    {
      return Builder.CreateFCmpUGE(left, right, "cmptmp");
    }
    else if (Op == "==")
    {
      return Builder.CreateFCmpUEQ(left, right, "cmptmp");
    }
    else if (Op == "!=")
    {
      return Builder.CreateFCmpUNE(left, right, "cmptmp");
    }
    else
    {
      return LogErrorV("invalid binary operator", Tok);
    }
  } // if both sides are bools
  else if (leftType == Type::getInt1Ty(TheContext) && rightType == Type::getInt1Ty(TheContext))
  {
    if (Op == "&&")
    {
      // lazy evaluation
      if (left == ConstantInt::get(TheContext, APInt(0, 0, true)))
      {
        return left;
      }
      else if (right == ConstantInt::get(TheContext, APInt(0, 0, true)))
      {
        return right;
      }
      else
      {
        return Builder.CreateAnd(left, right, "andtmp");
      }
    }
    else if (Op == "||")
    {
      // lazy evaluation
      if (left == ConstantInt::get(TheContext, APInt(1, 0, true)))
      {
        return left;
      }
      else if (right == ConstantInt::get(TheContext, APInt(1, 0, true)))
      {
        return right;
      }
      else
      {
        return Builder.CreateOr(left, right, "ortmp");
      }
    }
    else if (Op == "==")
    {
      return Builder.CreateICmpEQ(left, right, "cmptmp");
    }
    else if (Op == "!=")
    {
      return Builder.CreateICmpNE(left, right, "cmptmp");
    }
    else
    {
      return LogErrorV("Invalid binary operator", Tok);
    }
  }
  else
  {
    return LogErrorV("Type of the left and right side of the binary expression does not match", Tok);
  }
}

Value *FunctionCallASTnode::codegen()
{
  // Look up the name in the global module table.
  Function *CalleeF = TheModule->getFunction(Name);
  if (!CalleeF)
  {
    return LogErrorV("Unknown function referenced", Tok);
  }
  // If there is an argument mismatch error.
  if (CalleeF->arg_size() != Args.size())
  {
    return LogErrorV("Incorrect number of arguments passed", Tok);
  }

  // generate the arguments and push onto the stack
  std::vector<Value *> ArgsV;
  for (unsigned i = 0, e = Args.size(); i != e; ++i)
  {
    ArgsV.push_back(Args[i]->codegen());
    if (!ArgsV.back())
      return nullptr;
  }

  // check argument value types are the same as in the call
  for (unsigned i = 0, e = Args.size(); i != e; ++i)
  {
    if (ArgsV[i]->getType() != CalleeF->getFunctionType()->getParamType(i))
    {
      if (ArgsV[i]->getType() == Type::getInt32Ty(TheContext) && CalleeF->getFunctionType()->getParamType(i) == Type::getFloatTy(TheContext))
      {
        ArgsV[i] = Builder.CreateSIToFP(ArgsV[i], Type::getFloatTy(TheContext), "casttmp");
        fprintf(stderr, "WARNING: Implicit assignment of function argument from int to float\n"); // parameter name
      }
      else if (ArgsV[i]->getType() == Type::getFloatTy(TheContext) && CalleeF->getFunctionType()->getParamType(i) == Type::getInt32Ty(TheContext))
      {
        ArgsV[i] = Builder.CreateFPToSI(ArgsV[i], Type::getInt32Ty(TheContext), "casttmp");
        fprintf(stderr, "WARNING: Explicit assignment of function argument from int to float\n"); // parameter name
      }
      else
      {
        return LogErrorV("Incorrect function argument type", Tok); // name and parameter
      }
    }
  }
  return Builder.CreateCall(CalleeF, ArgsV, "calltmp");
}

Value *BlockASTnode::codegen()
{
  // increment the level of the scope
  level++;
  // create a new map for the new scope
  VariableStack[level] = std::map<std::string, AllocaInst *>();
  // generate the code for the local declarations and the statements
  for (auto &i : local_decls)
  {
    i->codegen();
  }

  for (auto &i : statements)
  {
    if (i != nullptr)
    {
      i->codegen();
    }
  }

  // pop the variables off of the stack
  VariableStack[level].clear();
  level--;
  return nullptr;
}

Value *WhileASTnode::codegen()
{
  // increment the level of the scope
  level++;
  // create a new map for the new scope
  VariableStack[level] = std::map<std::string, AllocaInst *>();

  Function *TheFunction = Builder.GetInsertBlock()->getParent();

  // create the basic blocks for the condition, loop block and the end of the loop
  BasicBlock *cond = BasicBlock::Create(TheContext, "cond", TheFunction);
  BasicBlock *loop = BasicBlock::Create(TheContext, "loop");
  BasicBlock *end_ = BasicBlock::Create(TheContext, "afterloop");

  // create the branch to the loop condition
  Builder.CreateBr(cond);
  Builder.SetInsertPoint(cond);
  Value *condV = Condition->codegen(); // generate the condition
  if (!condV)
    return nullptr;
  Value *comp = Builder.CreateICmpNE(condV, ConstantInt::get(TheContext, APInt(1, 0, false)), "ifcond");
  Builder.CreateCondBr(comp, loop, end_); // create the branch to the loop or the end of the loop

  // create the loop block
  TheFunction->getBasicBlockList().push_back(loop);
  Builder.SetInsertPoint(loop);
  Stmt->codegen();        // generate the loop body
  Builder.CreateBr(cond); // create the branch to the condition

  TheFunction->getBasicBlockList().push_back(end_);
  Builder.SetInsertPoint(end_);

  // pop the variables off of the stack
  VariableStack[level].clear();
  level--;

  return Constant::getNullValue(Type::getInt32Ty(TheContext));
}

Value *IfASTnode::codegen()
{
  if (ElseBlock == nullptr)
  { // if no else block

    Value *cond = IfCondition->codegen();
    Value *comp = Builder.CreateICmpNE(cond, ConstantInt::get(TheContext, APInt(1, 0, false)), "ifcond");

    Function *TheFunction = Builder.GetInsertBlock()->getParent();
    // create blocks for if the condition is true and the end of the if statement
    BasicBlock *true_ =
        BasicBlock::Create(TheContext, "ifthen", TheFunction);
    BasicBlock *end_ = BasicBlock::Create(TheContext, "end");

    // create conditional branch between the true block and the end block
    Builder.CreateCondBr(comp, true_, end_);
    // set the insertion point to the true block
    Builder.SetInsertPoint(true_);
    Value *ifV = IfBlock->codegen();

    Builder.CreateBr(end_);
    TheFunction->getBasicBlockList().push_back(end_);
    Builder.SetInsertPoint(end_);
    return Constant::getNullValue(Type::getInt32Ty(TheContext));
    ;
  }
  else
  { // if else block
    // create conditional branch between the true block and the false block
    Value *cond = IfCondition->codegen();
    if (!cond)
      return nullptr;
    if (cond->getType() != Type::getInt1Ty(TheContext))
    {
      return LogErrorV("If statement condition must be a 'bool'", Tok);
    }
    Value *comp = Builder.CreateICmpNE(cond, ConstantInt::get(TheContext, APInt(1, 0, false)), "ifcond");

    Function *TheFunction = Builder.GetInsertBlock()->getParent();
    // create blocks for if the condition is true, false and when they merge
    BasicBlock *true_ =
        BasicBlock::Create(TheContext, "ifthen", TheFunction);
    BasicBlock *false_ =
        BasicBlock::Create(TheContext, "elsethen");
    BasicBlock *merge = BasicBlock::Create(TheContext, "cont");

    Builder.CreateCondBr(comp, true_, false_);

    // set the insertion point to the true block and branch to the merge block
    Builder.SetInsertPoint(true_);

    Value *ifV = IfBlock->codegen();
    Builder.CreateBr(merge);
    // set the insertion point to the false block and branch to the merge block
    TheFunction->getBasicBlockList().push_back(false_);
    Builder.SetInsertPoint(false_);

    Value *elseV = ElseBlock->codegen();
    Builder.CreateBr(merge);

    TheFunction->getBasicBlockList().push_back(merge);
    // set the insertion point to the merge block
    Builder.SetInsertPoint(merge);
    return Constant::getNullValue(Type::getInt32Ty(TheContext)); // dont know why this is here tbh
  }
};

Value *AssignASTnode::codegen()
{
  // evaluate the rhs
  Value *V = RHS->codegen();
  if (!V)
    return nullptr;
  // look up the value in the local symbol table
  bool found = false;
  for (int i = level; i >= 0; i--)
  {
    // update the variables in the stack in all the levels of the local declarations
    if (VariableStack[i][Name])
    { // if the local variable is found
      // allow for implicit AND explicict assignment
      if (V->getType() != VariableStack[i][Name]->getAllocatedType())
      {
        if ((V->getType() == Type::getInt32Ty(TheContext)) && (VariableStack[i][Name]->getAllocatedType() == Type::getFloatTy(TheContext)))
        {
          fprintf(stderr, "WARNING: Implicit assignment of local variable from int to float\n");
          V = Builder.CreateSIToFP(V, Type::getFloatTy(TheContext), "tmp");
        }
        else if ((V->getType() == Type::getFloatTy(TheContext)) && (VariableStack[i][Name]->getAllocatedType() == Type::getInt32Ty(TheContext)))
        {
          fprintf(stderr, "WARNING: Implicit assignment of local variable from int to float\n");
          V = Builder.CreateFPToSI(V, Type::getInt32Ty(TheContext), "tmp");
        }
        else
        {
          return LogErrorV("Type of local variable and expression do not match", Tok);
        }
      }
      Builder.CreateStore(V, VariableStack[i][Name]);
      found = true;
    }
  }
  // if the variable is global
  if (!found)
  {
    if (GlobalVariable *g = TheModule->getGlobalVariable(Name))
    {
      //attempt to cast the value to the global variable type
      
      // if(V->getType() != g->getType()->getPointerElementType()){
      //   fprintf(stderr, "global - implicit assignment from int to float\n");
      //   V = Builder.CreateSIToFP(V, Type::getFloatTy(TheContext), "tmp");
      // }else if((V->getType() == Type::getFloatTy(TheContext)) && (g->getType() == Type::getInt32PtrTy(TheContext))){
      //   fprintf(stderr, "global - explicit assignment from float to int\n");
      //   V = Builder.CreateFPToSI(V, Type::getInt32Ty(TheContext), "tmp");
      // }else{
      //   return LogErrorV("assignment type mismatch", Tok);
      // }
      Builder.CreateStore(V, g);
      found = true;
    }
  }

  if (!found)
  {
    return LogErrorV("Unknown variable name called", Tok);
  }

  return V;
}

Function *PrototypeASTnode::codegen()
{
  // create a vector of the types of the parameters
  std::vector<Type *> types;
  for (auto &i : Params)
  {
    if (i->getType() == "float")
    {
      types.push_back(Type::getFloatTy(TheContext));
    }
    else if (i->getType() == "int")
    {
      types.push_back(Type::getInt32Ty(TheContext));
    }
    else if (i->getType() == "bool")
    {
      types.push_back(Type::getInt1Ty(TheContext));
    }
  }

  Function *F = nullptr;
  // create the function type and add it to the module
  if (Type_spec == "float")
  {
    FunctionType *FT = FunctionType::get(Type::getFloatTy(TheContext), types, false);
    F = Function::Create(FT, Function::ExternalLinkage, Name, TheModule.get());
  }
  else if (Type_spec == "int")
  {
    FunctionType *FT = FunctionType::get(Type::getInt32Ty(TheContext), types, false);
    F = Function::Create(FT, Function::ExternalLinkage, Name, TheModule.get());
  }
  else if (Type_spec == "bool")
  {
    FunctionType *FT = FunctionType::get(Type::getInt1Ty(TheContext), types, false);
    F = Function::Create(FT, Function::ExternalLinkage, Name, TheModule.get());
  }
  else if (Type_spec == "void")
  {
    FunctionType *FT = FunctionType::get(Type::getVoidTy(TheContext), types, false);
    F = Function::Create(FT, Function::ExternalLinkage, Name, TheModule.get());
  }
  else
  {
    return LogErrorF("Unknown function return type", Tok);
  }

  // set the names of the parameters
  unsigned i = 0;
  for (auto &arg : F->args())
    arg.setName(Params[i++]->getName());

  return F;
};

Function *ExternASTnode::codegen()
{
  // create the prototype
  std::unique_ptr<PrototypeASTnode> Prototype = std::make_unique<PrototypeASTnode>(Tok, Name, std::move(Params), Type);
  auto &P = *Prototype;
  // check the function doesnt already exist
  if (TheModule->getFunction(Prototype->getName()))
    return LogErrorF("Function has already been defined", Tok);
  // generate code for the extern prototype
  return Prototype->codegen();
};

Function *FunDeclASTnode::codegen()
{
  auto &P = *Prototype;
  Function *TheFunction = TheModule->getFunction(Prototype->getName());
  // if the function doesnt exist, create it
  if (!TheFunction)
    TheFunction = Prototype->codegen();
  // check the function creaition was successful
  if (!TheFunction)
  {
    return nullptr;
  }
  // create a new basic block to start insertion into
  BasicBlock *BB = BasicBlock::Create(TheContext, "entry", TheFunction);
  Builder.SetInsertPoint(BB);

  // create a new scope to add the parameters to
  level++;
  VariableStack[level] = std::map<std::string, AllocaInst *>(); 

  // create the arguments
  for (auto &Arg : TheFunction->args())
  {
    // create an alloca for the argument
    AllocaInst *Alloca = CreateEntryBlockAlloca(TheFunction, Arg.getName(), Arg.getType());
    // store the argument in the alloca
    Builder.CreateStore(&Arg, Alloca);
    // add the argument to the symbol table
    VariableStack[level][std::string(Arg.getName())] = Alloca;
  }

  // generate the body of the function
  if (Value *RetVal = Block->codegen())
  {
    // finish off the function
    Builder.CreateRet(RetVal);
  }
  // validate the generated code, checking for consistency
  verifyFunction(*TheFunction);
  // return the function
  VariableStack[level].clear();
  level--;
  return TheFunction;
};

Value *ReturnASTnode::codegen()
{
  // get the function
  if (Function *TheFunction = Builder.GetInsertBlock()->getParent())
  {
    // get the return type of the function
    Type *RetType = TheFunction->getReturnType();
    if (ReturnExpression != nullptr)
    {
      Value *v = ReturnExpression->codegen();
      if (v->getType() != RetType)
      { // check the actual return type matches the expected return type
        if (v->getType() == Type::getInt32Ty(TheContext) && RetType == Type::getFloatTy(TheContext))
        {
          fprintf(stderr, "WARNING: Implicit return from int to float\n");
          v = Builder.CreateSIToFP(v, Type::getFloatTy(TheContext), "tmp");
        }
        else if (v->getType() == Type::getFloatTy(TheContext) && RetType == Type::getInt32Ty(TheContext))
        {
          fprintf(stderr, "WARNING: Explicit return from float to int\n");
          v = Builder.CreateFPToSI(v, Type::getInt32Ty(TheContext), "tmp");
        }
        else
        {
          return LogErrorV("Return type does not match the function definition", Tok);
        }
      }
      Builder.CreateRet(v);
      return v;
    }
    else
    { // if there is no return expression, return void
      if (RetType == Type::getVoidTy(TheContext))
      {
        Builder.CreateRetVoid();
        return nullptr;
      }
      else
      {
        return LogErrorV("Return type does not match the function definition", Tok);
      }
    }
  }
  else
  {
    return LogErrorV("Return statement outside of a function", Tok);
  }
};

Value *ProgramASTnode::codegen()
{
  VariableStack[0] = std::map<std::string, AllocaInst *>(); // create the first level of the variable map
  for (auto &i : Extern_list)
  { // generate code for the externs
    i->codegen();
  }

  for (auto &i : Decl_list)
  { // generate code for the declarations
    i->codegen();
  }

  return nullptr;
};

//===----------------------------------------------------------------------===//
// AST Printer
//===----------------------------------------------------------------------===//

inline llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                                     const ASTnode &ast)
{
  os << ast.to_string();
  return os;
}

//===----------------------------------------------------------------------===//
// Main driver code.
//===----------------------------------------------------------------------===//

int main(int argc, char **argv)
{
  if (argc == 2)
  {
    pFile = fopen(argv[1], "r");
    if (pFile == NULL)
      perror("Error opening file");
  }
  else
  {
    std::cout << "Usage: ./code InputFile\n";
    return 1;
  }

  // initialize line number and column numbers to zero
  lineNo = 1;
  columnNo = 1;

  // Make the module, which holds all the code.
  TheModule = std::make_unique<Module>("mini-c", TheContext);

  // Run the parser now.
  getNextToken();
  fprintf(stderr, "BEGIN PARSING\n");
  std::unique_ptr<ASTnode> program = parser();
  fprintf(stderr, "PARSING FINISHED\nBEGIN PRINTING\n\n");
  llvm::outs() << *program << "\n";
  fprintf(stderr, "\nPRINTING FINISHED\nBEGIN CODE GENERATION\n");
  Value *v = program->codegen();
  fprintf(stderr, "CODE GENERATION FINISHED\n");

  //********************* Start printing final IR **************************
  // Print out all of the generated code into a file called output.ll
  auto Filename = "output.ll";
  std::error_code EC;
  raw_fd_ostream dest(Filename, EC, sys::fs::OF_None);

  if (EC)
  {
    errs() << "Could not open file: " << EC.message();
    return 1;
  }
  printf("\n");
  // TheModule->print(errs(), nullptr); // print IR to terminal
  TheModule->print(dest, nullptr);
  //********************* End printing final IR ****************************

  fclose(pFile); // close the file that contains the code that was parsed
  return 0;
}
