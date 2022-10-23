#include "mccomp.hh"

using namespace llvm;
using namespace llvm::sys;
using namespace std;

//===----------------------------------------------------------------------===//
// AST nodes
//===----------------------------------------------------------------------===//

std::string LocalDecl::to_string() const {
    auto out = std::string("LocalDecl[type=");
    out += type.lexeme;
    out += std::string(", ident=");
    out += ident.lexeme;
    out += std::string("]\n\t");
    return out;
}

// class LocalDecl: public ASTNode {
//     TOKEN type;
//     TOKEN ident;
// public:
//     LocalDecl(TOKEN type, TOKEN ident): type(type), ident(ident) {}
    Value* LocalDecl::codegen() {};
//     std::string to_string() const {
//         auto out = std::string("LocalDecl[type=");
//         out += type.lexeme;
//         out += std::string(", ident=");
//         out += ident.lexeme;
//         out += std::string("]\n\t");
//         return out;
//     }
// };

// class Block: public ASTNode {
//     vector<unique_ptr<ASTNode>> local_decls;
//     vector<unique_ptr<ASTNode>> stmt_list;
// public:
//     Block(vector<unique_ptr<ASTNode>> local_decls, vector<unique_ptr<ASTNode>> stmt_list): local_decls(std::move(local_decls)), stmt_list(std::move(stmt_list)) {}
    Value* Block::codegen() {};
    std::string Block::to_string() const {
        auto out = std::string("Block[\n\tlocal_decls=");
        for (auto &decl: local_decls) {
            out += decl->to_string();
        }
        out += std::string("stmt_list=");
        for (auto &stmt : stmt_list) {
            out += stmt->to_string();
            out += std::string("\n\t");
        }
        out += std::string("]");
        return out;
    }
// };

// class Param: public ASTNode {
//     TOKEN type;
//     TOKEN ident;
// public:
//     Param(TOKEN type, TOKEN ident): type(type), ident(ident) {}
    Value* Param::codegen() {};
    std::string Param::to_string() const {
        auto out = std::string("Param[type=");
        out += type.lexeme;
        out += std::string(", ident=");
        out += ident.lexeme;
        out += std::string("]");
        return out;
    }
// };

// class ExternDecl: public ASTNode {
//     TOKEN type;
//     TOKEN ident;
//     vector<unique_ptr<ASTNode>> params;
// public:
//     ExternDecl(TOKEN type, TOKEN ident, vector<unique_ptr<ASTNode>> params): type(type), ident(ident), params(std::move(params)) {}
    Value* ExternDecl::codegen() {}
    std::string ExternDecl::to_string() const {
        auto out = std::string("Extern[type=");
        out += type.lexeme;
        out += std::string(", id=");
        out += ident.lexeme;

        out += std::string(", params=[");
        for (auto &param : params) {
            out += param->to_string();
            out += std::string(" ");
        }
        out += std::string("]]\n\t");
        return out;
    }
// };

// class GlobalDecl: public ASTNode {
//     TOKEN type;
//     TOKEN ident;
//     // vvvv optional vvv
//     vector<unique_ptr<ASTNode>> params;
//     unique_ptr<ASTNode> block;
// public:
//     GlobalDecl(TOKEN type, TOKEN ident): type(type), ident(ident) {}
//     GlobalDecl(TOKEN type, TOKEN ident, vector<unique_ptr<ASTNode>> params, unique_ptr<ASTNode> block): type(type), ident(ident), params(std::move(params)), block(std::move(block)) {}
    Value* GlobalDecl::codegen() {}
    std::string GlobalDecl::to_string() const {
        auto out = std::string("GlobalDecl[type=");
        out += type.lexeme;
        out += std::string(", id=");
        out += ident.lexeme;

        if (block != nullptr) {
            out += std::string(", params=[");
            for (auto &param : params) {
                out += param->to_string();
                out += std::string(" ");
            }
            out += std::string("], block=");
            out += block->to_string();
        }
        out += std::string("]\n\t");
        return out;
    }
// };

// class Program: public ASTNode {
//     vector<unique_ptr<ASTNode>> extern_list;
//     vector<unique_ptr<ASTNode>> decl_list;
// public:
//     Program(vector<unique_ptr<ASTNode>> extern_list, vector<unique_ptr<ASTNode>> decl_list): extern_list(std::move(extern_list)), decl_list(std::move(decl_list)) {}
    Value* Program::codegen() {};
    std::string Program::to_string() const {
        auto out = std::string("Program[\n\texterns=");
        for (auto &ext : extern_list) {
            out += ext->to_string();
        }
        out += std::string("\n\tdecls=");
        for (auto &decl : decl_list) {
            out += decl->to_string();
        }
        out += std::string("]\n");
        return out;
    }
// };

// class ReturnStmt: public ASTNode {
//     unique_ptr<ASTNode> expr;
// public:
//     ReturnStmt(unique_ptr<ASTNode> expr): expr(std::move(expr)) {}
    Value* ReturnStmt::codegen() {}
    std::string ReturnStmt::to_string() const {
        auto out = std::string("Return[");
        if (expr != nullptr) {
            out += expr->to_string();
        }
        out += std::string("]");
        return out;
    }
// };

// class IfStmt: public ASTNode {
//     unique_ptr<ASTNode> expr;
//     unique_ptr<ASTNode> block;
//     unique_ptr<ASTNode> else_block;
// public:
//     IfStmt(unique_ptr<ASTNode> expr, unique_ptr<ASTNode> block, unique_ptr<ASTNode> else_block): expr(std::move(expr)), block(std::move(block)), else_block(std::move(else_block)) {}
    Value* IfStmt::codegen() {};
    std::string IfStmt::to_string() const {
        auto out = std::string("IfStmt[guard=");
        
        out += expr->to_string();
        out += std::string(", block=");
        out += block->to_string();
        if (else_block) {
            out += std::string(", else=");
            out += else_block->to_string();   
        }
        out += std::string("]");
        return out;
    }
// };

// class WhileStmt: public ASTNode {
//     unique_ptr<ASTNode> expr;
//     unique_ptr<ASTNode> block;
// public:
//     WhileStmt(unique_ptr<ASTNode> expr, unique_ptr<ASTNode> block): expr(std::move(expr)), block(std::move(block)) {}
    Value* WhileStmt::codegen() {};
    std::string WhileStmt::to_string() const {
        auto out = std::string("WhileStmt[guard=");
        if (expr != nullptr) {        
            out += expr->to_string();
        }
        out += std::string(", block=");
        if (block != nullptr) {
            out += block->to_string();
        }
        out += std::string("]");
        return out;
    }
// };

// class AssignExpr: public ASTNode {
//     TOKEN ident;
//     unique_ptr<ASTNode> expr;
// public:
//     AssignExpr(TOKEN ident, unique_ptr<ASTNode> expr): ident(ident), expr(std::move(expr)) {}
    Value* AssignExpr::codegen() {};
    std::string AssignExpr::to_string() const {
        auto out = std::string("AssignExpr[id=");
        out += ident.lexeme;
        out += std::string(", expr=");
        out += expr->to_string();
        out += std::string("]");
        return out;
    }
// };

// class BinaryOp: public ASTNode {
//     unique_ptr<ASTNode> lexpr;
//     TOKEN op;
//     unique_ptr<ASTNode> rexpr;
// public:
//     BinaryOp(unique_ptr<ASTNode> lexpr, TOKEN op, unique_ptr<ASTNode> rexpr): lexpr(std::move(lexpr)), op(op), rexpr(std::move(rexpr)) {}
    Value* BinaryOp::codegen() {}
    std::string BinaryOp::to_string() const {
        auto out = std::string("BinaryOp[left=");
        out += lexpr->to_string();
        out += std::string(", op=");
        out += op.lexeme;
        out += std::string(", right=");
        out += rexpr->to_string();
        out += std::string("]");
        return out;
    }
// };

// class UnaryOp: public ASTNode {
//     unique_ptr<ASTNode> expr;
//     TOKEN op;
// public:
//     UnaryOp(TOKEN op, unique_ptr<ASTNode> expr): op(op), expr(std::move(expr)) {}
    Value* UnaryOp::codegen() {};
    std::string UnaryOp::to_string() const {
        auto out = std::string("Unary[op=");
        out += op.lexeme;
        out += std::string(", expr=");
        out += expr->to_string();
        out += std::string("]");
        return out;
    }
// };

// /// IntASTnode - Class for integer literals like 1, 2, 10,
// class IntLit : public ASTNode {
//   int val;
//   TOKEN tok;
// public:
//     IntLit(TOKEN tok, int val): val(val), tok(tok) {}
    Value* IntLit::codegen() {}
    std::string IntLit::to_string() const {
        return std::string("IntLit[val=") + std::to_string(val) + std::string("]");
    }
// };

// class FloatLit: public ASTNode {
//     float val;
//     TOKEN tok;
// public:
//     FloatLit(TOKEN tok, float val): tok(tok), val(val) {}
    Value* FloatLit::codegen() {}
    std::string FloatLit::to_string() const {
        return std::string("FloatLit[val=") + std::to_string(val) + std::string("]");
    }
// };

// class BoolLit: public ASTNode {
//     bool val;
//     TOKEN tok;
// public:
//     BoolLit(TOKEN tok, bool val): tok(tok), val(val) {}
    Value* BoolLit::codegen() {}
    std::string BoolLit::to_string() const {
        return std::string("BoolLit[val=") + std::to_string(val) + std::string("]");
    }
// };

// class VariableLit: public ASTNode {
//     TOKEN tok;
// public:
//     VariableLit(TOKEN tok) : tok(tok) {}
    Value* VariableLit::codegen() {}
    std::string VariableLit::to_string() const {
        return std::string("Variable[ident=") + tok.lexeme + std::string("]");
    }
// };

// class FunctionCallLiteral: public ASTNode {
//     TOKEN ident;
//     vector<unique_ptr<ASTNode>> args;
// public:
//     FunctionCallLiteral(TOKEN ident, vector<unique_ptr<ASTNode>> args) : ident(ident), args(std::move(args)) {}
    Value* FunctionCallLiteral::codegen() {}
    std::string FunctionCallLiteral::to_string() const {
        std::string out = std::string("FunctionCallLiteral[name=\"");
        out += ident.lexeme;
        out += std::string("\", args=[");
        for (auto &arg : args) {
            out += arg->to_string();
            out += std::string(" ");
        }
        out += std::string("]]");
        return out;
    }
// };

// class Noop: public ASTNode {
// public: 
//     Noop() {}
    Value* Noop::codegen() {}
    std::string Noop::to_string() const {
        return std::string("[noop]");
    }
// };