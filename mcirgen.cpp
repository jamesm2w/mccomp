#include "mccomp.hh"

using namespace llvm;
using namespace llvm::sys;

template<typename... Args>
Value* ThrowValueError( const std::string& format, Args... args ) {

    int size_s = std::snprintf( nullptr, 0, "%s", format.c_str(), args... ) + 1; // Extra space for '\0'
    if( size_s <= 0 ){ fprintf(stderr, "Error in formating error message"); return nullptr; }
    auto size = static_cast<size_t>( size_s );
    std::unique_ptr<char[]> buf( new char[ size ] );
    std::snprintf( buf.get(), size, "%s", format.c_str(), args... );

    fprintf(stderr, "Codegen Error ");
    fprintf(stderr, "%s", std::string( buf.get(), buf.get() + size - 1 ).c_str()); // We don't want the '\0' inside
    fprintf(stderr, "\n");

    return nullptr;
}

Value* Noop::codegen() {
    return 0;
}

Value* BoolLit::codegen() {
    if (val) {
        return ConstantInt::get(TheContext, APInt(1, 1));
    } else {
        return ConstantInt::get(TheContext, APInt(1, 0));
    }
}

Value* FloatLit::codegen() {
    return ConstantFP::get(TheContext, APFloat(val));
}

Value* IntLit::codegen() {
    return ConstantInt::get(TheContext, APInt(24, val));
}

Value* VariableLit::codegen() {
    Value *V = NamedValues[tok.lexeme];
    if (!V) {
        ThrowValueError("Unknown variable name %s", tok.lexeme.c_str());
    }

    return V;
}

Value* FunctionCallLiteral::codegen() {
    return ThrowValueError("TODO: Implement FunctionCallLiteral codegen");
}

Value* UnaryOp::codegen() {
    return ThrowValueError("TODO: Implement Unary Operator codegen");
}

Value* BinaryOp::codegen() {
    return ThrowValueError("TODO: Implement Binary Operator codegen");
}

Value* AssignExpr::codegen() {
    return ThrowValueError("TODO: Implemenet Asssignment Expr codegen");
};

Value* LocalDecl::codegen() {};

Value* Block::codegen() {};

Value* Param::codegen() {};

Value* ExternDecl::codegen() {}

Value* GlobalDecl::codegen() {}

Value* Program::codegen() {};

Value* ReturnStmt::codegen() {}

Value* IfStmt::codegen() {};

Value* WhileStmt::codegen() {};
