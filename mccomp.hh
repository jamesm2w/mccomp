#pragma once

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
#include "llvm/MC/TargetRegistry.h"
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
#include <optional>

extern FILE *pFile;

extern std::string IdentifierStr; // Filled in if IDENT
extern int IntVal;                // Filled in if INT_LIT
extern bool BoolVal;              // Filled in if BOOL_LIT
extern float FloatVal;            // Filled in if FLOAT_LIT
extern int lineNo;
extern int columnNo;

// LEXER DEFINITIONS

// The lexer returns one of these for known things.
enum TOKEN_TYPE {

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
struct TOKEN {
  int type = -100;
  std::string lexeme;
  int lineNo;
  int columnNo;
};

TOKEN returnTok(std::string lexVal, int tok_type);

TOKEN gettok();

// PARSER DEFINITIONS

static std::map<int, int> BinopPrecedence = {
  {OR,  10},
  {AND, 20},
  {EQ,  30},
  {NE, 40},
  {LE, 50},
  {LT,  60},
  {GE, 70},
  {GT,  80},
  {PLUS, 90},
  {MINUS, 100},
  {ASTERIX, 110},
  {DIV, 120},
  {MOD, 130}
};

static TOKEN CurTok;
static std::deque<TOKEN> tok_buffer;

 TOKEN getNextToken();

 void putBackToken(TOKEN tok);

// AST DEFINITIONS

#include "mcast.hh"

std::unique_ptr<Program> parser();

int parse_extern_list(std::vector<std::unique_ptr<ExternDecl>> &list);
std::unique_ptr<ExternDecl> parse_extern();

int parse_decl_list(std::vector<std::unique_ptr<GlobalDecl>> &list);
std::unique_ptr<GlobalDecl> parse_decl();

std::optional<std::vector<std::unique_ptr<Param>>> parse_params();
int parse_param_list(std::vector<std::unique_ptr<Param>> &list);
std::unique_ptr<Param> parse_param();

std::unique_ptr<Block> parse_block();

int parse_local_decls(std::vector<std::unique_ptr<LocalDecl>> &list);
std::unique_ptr<LocalDecl> parse_local_decl();

int parse_stmt_list(std::vector<std::unique_ptr<ASTNode>> &list);
std::unique_ptr<ASTNode> parse_stmt();

std::unique_ptr<ASTNode> parse_expr_stmt();
std::unique_ptr<WhileStmt> parse_while_stmt();
std::unique_ptr<IfStmt> parse_if_stmt();
std::unique_ptr<ReturnStmt> parse_return_stmt();
std::unique_ptr<ASTNode> parse_expr();
std::unique_ptr<ASTNode> parse_primary();
std::unique_ptr<ASTNode> parse_literal();
std::unique_ptr<ASTNode> parse_bin_op_rhs(int exprPrec, std::unique_ptr<ASTNode> LHS);

int parse_args(std::vector<std::unique_ptr<ASTNode>> &list);