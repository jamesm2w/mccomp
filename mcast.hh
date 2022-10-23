#pragma once

#include "mccomp.hh"

using namespace llvm;
using namespace llvm::sys;
using namespace std;

/// ASTnode - Base class for all AST nodes.
class ASTNode {
public:
  virtual ~ASTNode() {}
  virtual llvm::Value *codegen() {};
  virtual std::string to_string() const { return "[ASTNode]"; };
};

class LocalDecl: public ASTNode {
    TOKEN type;
    TOKEN ident;
public:
    LocalDecl(TOKEN type, TOKEN ident): type(type), ident(ident) {}
    Value *codegen() override;
    std::string to_string() const override;
};

class Block: public ASTNode {
    vector<unique_ptr<LocalDecl>> local_decls;
    vector<unique_ptr<ASTNode>> stmt_list;
public:
    Block(vector<unique_ptr<LocalDecl>> local_decls, vector<unique_ptr<ASTNode>> stmt_list): local_decls(std::move(local_decls)), stmt_list(std::move(stmt_list)) {}
    Value *codegen() override;
    std::string to_string() const override;
};

class Param: public ASTNode {
    TOKEN type;
    TOKEN ident;
public:
    Param(TOKEN type, TOKEN ident): type(type), ident(ident) {}
    Value *codegen() override;
    std::string to_string() const override;
};

class ExternDecl: public ASTNode {
    TOKEN type;
    TOKEN ident;
    vector<unique_ptr<Param>> params;
public:
    ExternDecl(TOKEN type, TOKEN ident, vector<unique_ptr<Param>> params): type(type), ident(ident), params(std::move(params)) {}
    Value *codegen() override;
    std::string to_string() const override;
};

class GlobalDecl: public ASTNode {
    TOKEN type;
    TOKEN ident;
    // vvvv optional vvv
    vector<unique_ptr<Param>> params;
    unique_ptr<Block> block;
public:
    GlobalDecl(TOKEN type, TOKEN ident): type(type), ident(ident) {}
    GlobalDecl(TOKEN type, TOKEN ident, vector<unique_ptr<Param>> params, unique_ptr<Block> block): type(type), ident(ident), params(std::move(params)), block(std::move(block)) {}
    Value *codegen() override;
    std::string to_string() const override;
};

class Program: public ASTNode {
    vector<unique_ptr<ExternDecl>> extern_list;
    vector<unique_ptr<GlobalDecl>> decl_list;
public:
    Program(vector<unique_ptr<ExternDecl>> extern_list, vector<unique_ptr<GlobalDecl>> decl_list): extern_list(std::move(extern_list)), decl_list(std::move(decl_list)) {}
    virtual Value *codegen() override;
    std::string to_string() const override;
};

class ReturnStmt: public ASTNode {
    unique_ptr<ASTNode> expr;
public:
    ReturnStmt(unique_ptr<ASTNode> expr): expr(std::move(expr)) {}
    virtual Value *codegen() override;
    std::string to_string() const override;
};

class IfStmt: public ASTNode {
    unique_ptr<ASTNode> expr;
    unique_ptr<Block> block;
    unique_ptr<Block> else_block;
public:
    IfStmt(unique_ptr<ASTNode> expr, unique_ptr<Block> block, unique_ptr<Block> else_block): expr(std::move(expr)), block(std::move(block)), else_block(std::move(else_block)) {}
    Value *codegen() override;
    std::string to_string() const override;
};

class WhileStmt: public ASTNode {
    unique_ptr<ASTNode> expr;
    unique_ptr<ASTNode> block;
public:
    WhileStmt(unique_ptr<ASTNode> expr, unique_ptr<ASTNode> block): expr(std::move(expr)), block(std::move(block)) {}
    Value *codegen() override;
    std::string to_string() const override;
};

class AssignExpr: public ASTNode {
    TOKEN ident;
    unique_ptr<ASTNode> expr;
public:
    AssignExpr(TOKEN ident, unique_ptr<ASTNode> expr): ident(ident), expr(std::move(expr)) {}
    Value *codegen() override;
    std::string to_string() const override;
};

class BinaryOp: public ASTNode {
    unique_ptr<ASTNode> lexpr;
    TOKEN op;
    unique_ptr<ASTNode> rexpr;
public:
    BinaryOp(unique_ptr<ASTNode> lexpr, TOKEN op, unique_ptr<ASTNode> rexpr): lexpr(std::move(lexpr)), op(op), rexpr(std::move(rexpr)) {}
    Value *codegen() override;
    std::string to_string() const override;
};

class UnaryOp: public ASTNode {
    unique_ptr<ASTNode> expr;
    TOKEN op;
public:
    UnaryOp(TOKEN op, unique_ptr<ASTNode> expr): op(op), expr(std::move(expr)) {}
    Value *codegen() override;
    std::string to_string() const override;
};

class IntLit : public ASTNode {
  int val;
  TOKEN tok;
public:
    IntLit(TOKEN tok, int val): val(val), tok(tok) {}
    Value *codegen() override;
    std::string to_string() const override;
};

class FloatLit: public ASTNode {
    float val;
    TOKEN tok;
public:
    FloatLit(TOKEN tok, float val): tok(tok), val(val) {}
    Value *codegen() override;
    std::string to_string() const override;
};

class BoolLit: public ASTNode {
    bool val;
    TOKEN tok;
public:
    BoolLit(TOKEN tok, bool val): tok(tok), val(val) {}
    Value *codegen() override;
    std::string to_string() const override;
};

class VariableLit: public ASTNode {
    TOKEN tok;
public:
    VariableLit(TOKEN tok) : tok(tok) {}
    Value *codegen() override;
    std::string to_string() const override;
};

class FunctionCallLiteral: public ASTNode {
    TOKEN ident;
    vector<unique_ptr<ASTNode>> args;
public:
    FunctionCallLiteral(TOKEN ident, vector<unique_ptr<ASTNode>> args) : ident(ident), args(std::move(args)) {}
    Value *codegen() override;
    std::string to_string() const override;
};

class Noop: public ASTNode {
public: 
    Noop() {}
    Value *codegen() override;
    std::string to_string() const override;
};