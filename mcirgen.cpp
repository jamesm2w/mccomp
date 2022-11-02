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

Type* getType(TOKEN tok) {
    switch (tok.type) {
        case BOOL_TOK:
            return Type::getInt1Ty(TheContext);
        case FLOAT_TOK:
            return Type::getFloatTy(TheContext);
        case INT_TOK:
            return Type::getInt32Ty(TheContext);
        case VOID_TOK:
            return Type::getVoidTy(TheContext);
        default:
            ThrowValueError("Invalid type \"%s\" expected bool, float, int void", tok.lexeme.c_str());
            return nullptr;
    }
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
    Value* operand = expr->codegen();
    if (!operand) {
        return nullptr;
    }

    switch (op.type) {
        case MINUS:
            // return Builder.CreateUnOp(SubOperator, operand, "unarysubtmp");
            // return ThrowValueError("TODO: Unary subtraction");
            return Builder.CreateFNeg(operand, "negtmp");
        case NOT:
            return Builder.CreateNeg(operand, "negtmp");
            // return ThrowValueError("TODO: Unary not");
        default:
            return ThrowValueError("Invalid Unary Operator \"%s\" expected \"!\", \"-\"", op.lexeme.c_str());
    }
}

Value* BinaryOp::codegen() {
    Value* left = lexpr->codegen();
    Value* right = rexpr->codegen();
    if (!left || !right) {
        return nullptr;
    }

    switch (op.type) {
        case PLUS:
            return Builder.CreateFAdd(left, right, "addtmp");
        case MINUS:
            return Builder.CreateFSub(left, right, "subtmp");
        case ASTERIX:
            return Builder.CreateFMul(left, right, "multmp");
        case DIV:
            return Builder.CreateFDiv(left, right, "divtmp");

        case LT:
        case LE:
        case GT:
        case GE:
        case EQ:
        case NE:
            return ThrowValueError("TODO: Implement operators");
    }
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
