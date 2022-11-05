#include "mccomp.hh"

using namespace llvm;
using namespace llvm::sys;
using namespace std;

// Tree Example
// Program 
// |- Extern Decl
// |  |- Extern Type=void Ident=add_x_y
// |  |  |- Param Type=int Ident=x
// |  |  |- Param Type=int Ident=y
// |  |- Extern Type=bool Ident=not
// |     |- Param Type=bool Ident=a
// ...

//===----------------------------------------------------------------------===//
// AST nodes
//===----------------------------------------------------------------------===//

// LocalDecl 
std::string LocalDecl::to_string(std::string prefix, std::string children_prefix) const {
    return prefix + "LocalDecl type=\"" + type.lexeme + "\" ident=" + ident.lexeme + "\"\n"; 
}

std::string Block::to_string(std::string prefix, std::string children_prefix) const {
    auto out = prefix + "Block\n";

    for (int i = 0; i < local_decls.size(); i++) {
        if (i == local_decls.size() - 1 && stmt_list.size() == 0) { // Last item and no stmts below it
            out += local_decls.at(i)->to_string(children_prefix + "└─ ", children_prefix + "   ");
        } else {
            out += local_decls.at(i)->to_string(children_prefix + "├─ ", children_prefix + "│  ");
        }
    }

    for (int i = 0; i < stmt_list.size(); i++) {
        if (i == stmt_list.size() - 1) { // Last Item
            out += stmt_list.at(i)->to_string(children_prefix + "└─ ", children_prefix + "   ");
        } else {
            out += stmt_list.at(i)->to_string(children_prefix + "├─ ", children_prefix + "│  ");
        }
    }

    return out;
}

std::string Param::to_string(std::string prefix, std::string children_prefix) const {
    return prefix + "Param type=\"" + type.lexeme + "\" ident=\"" + ident.lexeme + "\"\n";
}

std::string ExternDecl::to_string(std::string prefix, std::string children_prefix) const {
    auto out = prefix + "Extern type=\"" + type.lexeme + "\" ident=\"" + ident.lexeme + "\"\n";
    for (int i = 0; i < params.size(); i++) {
        if (i == params.size() - 1) { // Last Item
            out += params.at(i)->to_string(children_prefix + "└─ ", children_prefix + "   ");
        } else {
            out += params.at(i)->to_string(children_prefix + "├─ ", children_prefix + "│  ");
        }
    }
    return out;
}

std::string GlobalDecl::to_string(std::string prefix, std::string children_prefix) const {

    auto out = prefix + "GlobalDecl type=\"" + type.lexeme + "\" ident=\"" + ident.lexeme + "\"\n";

    if (block != nullptr) {
        for (int i = 0; i < params.size(); i++) {
            out += params.at(i)->to_string(children_prefix + "├─ ", children_prefix + "│  ");
        }
        // Block would always be the last item
        out += block->to_string(children_prefix + "└─ ", children_prefix + "   ");
    }

    return out;
}

std::string Program::to_string(std::string prefix, std::string children_prefix) const {
    auto out = prefix + "Program\n";
    
    for (int i = 0; i < extern_list.size(); i++) {
        if (i == extern_list.size() - 1 && decl_list.size() == 0) { // Last item and no stmts below it
            out += extern_list.at(i)->to_string(children_prefix + "└─ ", children_prefix + "   ");
        } else {
            out += extern_list.at(i)->to_string(children_prefix + "├─ ", children_prefix + "│  ");
        }
    }

    for (int i = 0; i < decl_list.size(); i++) {
        if (i == decl_list.size() - 1) { // Last Item
            out += decl_list.at(i)->to_string(children_prefix + "└─ ", children_prefix + "   ");
        } else {
            out += decl_list.at(i)->to_string(children_prefix + "├─ ", children_prefix + "│  ");
        }
    }

    return out;
}

std::string ReturnStmt::to_string(std::string prefix, std::string children_prefix) const {
    auto out = prefix + "Return\n";
    if (expr != nullptr && expr.has_value()) {
        out += expr->get()->to_string(children_prefix + "└─ ", children_prefix + "   ");
    }

    return out;
}

std::string IfStmt::to_string(std::string prefix, std::string children_prefix) const {
    auto out = prefix + "IfStmt\n";

    out += expr->to_string(children_prefix + "├─ ", children_prefix + "│  ");

    if (else_block != nullptr) {
        out += block->to_string(children_prefix + "├─ ", children_prefix + "│  ");
        out += else_block->to_string(children_prefix + "└─ ", children_prefix + "   ");
    } else {
        out += block->to_string(children_prefix + "└─ ", children_prefix + "   ");
    }

    return out;
}

std::string WhileStmt::to_string(std::string prefix, std::string children_prefix) const {
    auto out = prefix + "WhileStmt\n";
    out += expr->to_string(children_prefix + "├─ ", children_prefix + "│  ");
    out += block->to_string(children_prefix + "└─ ", children_prefix + "   ");

    return out;
}

std::string AssignExpr::to_string(std::string prefix, std::string children_prefix) const {
    auto out = prefix + "AssignExpr ident=\"" + ident.lexeme + "\"\n";
    out += expr->to_string(children_prefix + "└─ ", children_prefix + "   ");

    return out;
}

std::string BinaryOp::to_string(std::string prefix, std::string children_prefix) const {
    auto out = prefix + "BinaryOp op=\"" + op.lexeme + "\"\n";
    out += lexpr->to_string(children_prefix + "├─ ", children_prefix + "│  ");
    out += rexpr->to_string(children_prefix + "└─ ", children_prefix + "   ");
    
    return out;
}

std::string UnaryOp::to_string(std::string prefix, std::string children_prefix) const {
    auto out = prefix + "Unary op=\"" + op.lexeme +  "\"\n";
    out += expr->to_string(children_prefix + "└─ ", children_prefix + "   ");

    return out;
}

std::string IntLit::to_string(std::string prefix, std::string children_prefix) const {
    return prefix + "IntLit value=" + std::to_string(val) + "\n";
}

std::string FloatLit::to_string(std::string prefix, std::string children_prefix) const {
    return prefix + "FloatLit value=" + std::to_string(val) + "\n";
}

std::string BoolLit::to_string(std::string prefix, std::string children_prefix) const {
    return prefix + "BoolLit value=" + std::to_string(val) + "\n";
}

std::string VariableLit::to_string(std::string prefix, std::string children_prefix) const {
    return prefix + "Variable ident=\"" + tok.lexeme + "\"\n";
}

std::string FunctionCallLiteral::to_string(std::string prefix, std::string children_prefix) const {

    auto out = prefix + "FunctionCallLiteral ident=\"" + ident.lexeme + "\"\n";
    for (int i = 0; i < args.size(); i++) {
        if (i == args.size() - 1) { // Last Item
            out += args.at(i)->to_string(children_prefix + "└─ ", children_prefix + "   ");
        } else {
            out += args.at(i)->to_string(children_prefix + "├─ ", children_prefix + "│  ");
        }
    }

    return out;
}

std::string Noop::to_string(std::string prefix, std::string children_prefix) const {
    return prefix + "Empty AST Node\n";
}