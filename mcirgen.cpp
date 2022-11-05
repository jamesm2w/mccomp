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

AllocaInst* createEntryBlockAlloca(Function* TheFunction, Type* type, const std::string& varName) {
    IRBuilder<> TmpB(&TheFunction->getEntryBlock(), TheFunction->getEntryBlock().begin());
    return TmpB.CreateAlloca(type, 0, varName.c_str());
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
    return ConstantInt::get(TheContext, APInt(32, val));
}

Value* VariableLit::codegen() {
    AllocaInst *V = NamedValues[tok.lexeme];
    if (!V) {

        Value* GV = GlobalNamedValues[tok.lexeme];
        if (!GV) {
            return ThrowValueError("Unknown variable name \"%s\"", tok.lexeme.c_str()); 
        }
        GlobalVariable* gvar = TheModule->getNamedGlobal(tok.lexeme);
        return Builder.CreateLoad(Type::getVoidTy(TheContext), gvar, "loadgvar");
        // return Builder.CreateLoad(Type::getVoidTy(TheContext), GV, "loadgvar");
    }
    // TODO: Make this proper type
    return Builder.CreateLoad(Type::getVoidTy(TheContext), V, "loadvar");
}

Value* FunctionCallLiteral::codegen() {
    Function* callee = TheModule->getFunction(ident.lexeme);

    if (!callee) return ThrowValueError("Undefined function \"%s\"", ident.lexeme.c_str());
    if (callee->arg_size() != args.size()) return ThrowValueError("Length of function parameters and supplied arguments is different");

    std::vector<Value*> argsv;
    for (uint i = 0, e = args.size(); i != e; ++i) {
        argsv.push_back(args[i]->codegen());
        if (!argsv.back()) return nullptr;
    }

    return Builder.CreateCall(callee, argsv, "call_func");
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
    Value* ex = expr->codegen();
    if (!ex) {
        return nullptr;
    }
    AllocaInst* alloca = NamedValues[ident.lexeme];
    if (!alloca) {
        return ThrowValueError("Undefined identity \"%s\"", ident.lexeme.c_str());
    }

    return Builder.CreateStore(ex, alloca);
};

Value* LocalDecl::codegen() {
    NamedValues[ident.lexeme] = createEntryBlockAlloca(Builder.GetInsertBlock()->getParent(), getType(type), ident.lexeme);
    return NamedValues[ident.lexeme];
};

Value* Block::codegen() {
    Function* TheFunction = Builder.GetInsertBlock()->getParent();
    BasicBlock* block = BasicBlock::Create(TheContext, "block", TheFunction);
    BasicBlock* end = BasicBlock::Create(TheContext, "end");
    Builder.SetInsertPoint(block);

    for (auto & decl : local_decls) {
        decl->codegen();
    }
    for (auto & stmt : stmt_list) {
        stmt->codegen();
    }

    TheFunction->getBasicBlockList().push_back(end);
    Builder.CreateBr(end);
    Builder.SetInsertPoint(end);
};

// Only used syntactically. No need to generate code for the param.
Value* Param::codegen() {
    return nullptr;
};

Value* prototype_codegen(TOKEN type, TOKEN ident, vector<unique_ptr<Param>>& params) {
    std::vector<Type*> param_list;
    for (auto &param : params) {
        Type* type = getType(param->get_type());
        if (!type) return ThrowValueError("Invalid Type \"%s\"", param->get_type().lexeme.c_str());
        param_list.push_back(type);
    }

    Type* return_type = getType(type);
    if (!return_type) return ThrowValueError("Invalid Type \"%s\"", type.lexeme.c_str());

    FunctionType* FT = FunctionType::get(return_type, param_list, false);

    Function* F = Function::Create(FT, Function::ExternalLinkage, ident.lexeme, TheModule.get());

    uint idx = 0;
    for (auto &arg : F->args()) { // TODO: Fix segfault here, too many params??
        arg.setName(params[idx++]->get_name().lexeme);
    }
}

Value* ExternDecl::codegen() {
    return prototype_codegen(type, ident, params);
}

Value* GlobalDecl::codegen() {

    if (block) {
        // Function Declaration
        Function* TheFunction = TheModule->getFunction(ident.lexeme);

        if (!TheFunction) 
            TheFunction = (Function*) prototype_codegen(type, ident, params);
        
        if (!TheFunction) return nullptr;

        BasicBlock* BB = BasicBlock::Create(TheContext, "func_entry", TheFunction);
        Builder.SetInsertPoint(BB);

        NamedValues.clear();
        for (auto &arg : TheFunction->args()) {
            AllocaInst *alloca = createEntryBlockAlloca(TheFunction, arg.getType(), std::string(arg.getName()));
            Builder.CreateStore(&arg, alloca);
            NamedValues[std::string(arg.getName())] = alloca;
        }

        Value* block_v = block->codegen();
        if (!block_v) {
            TheFunction->eraseFromParent();
            return nullptr;
        }

        // Apparently should create a ret here but that should be managed by the code?
        verifyFunction(*TheFunction);
        return TheFunction;

    } else {
        // Global Variable Declaration
        if (GlobalNamedValues[ident.lexeme]) {
            return ThrowValueError("Global Variable \"%s\" is already defined", ident.lexeme.c_str());
        }
        
        Type* typ = getType(type);
        GlobalVariable* glob = new GlobalVariable(typ, false, GlobalValue::CommonLinkage);

        // Value* val = TheModule->getOrInsertGlobal(ident.lexeme.c_str(), typ);
        
        GlobalNamedValues[ident.lexeme] = glob;
        return glob;
    } 
}

Value* Program::codegen() {
    for (auto & extern_decl : extern_list) {
        if (!extern_decl->codegen()) return nullptr;
    }

    for (auto & decl : decl_list) {
        if (!decl->codegen()) return nullptr;
    }
    
    return Builder.GetInsertBlock();
};

Value* ReturnStmt::codegen() {
    if (expr.has_value()) {
        Value* val = expr->get()->codegen();
        if (!val) return nullptr;
        return Builder.CreateRet(val);
    } else {
        return Builder.CreateRetVoid();
    }
}

Value* IfStmt::codegen() {
    Function* TheFunction = Builder.GetInsertBlock()->getParent();
    BasicBlock* true_ = BasicBlock::Create(TheContext, "branchtrue", TheFunction);
    
    BasicBlock* end_;
    if (else_block)
        end_ = BasicBlock::Create(TheContext, "branchelse");
    else 
        end_ = BasicBlock::Create(TheContext, "branchend");
    
    Value* cond = expr->codegen();
    if (!cond) {
        return nullptr;
    }

    Value* comp = Builder.CreateICmpNE(cond, ConstantInt::get(TheContext, APInt(1, 1)), "ifcond");
    Builder.CreateCondBr(comp, true_, end_);
    
    Builder.SetInsertPoint(true_);
    Value* true_block_v = block->codegen();

    if (!true_block_v) return nullptr;

    if (else_block) {
        Builder.SetInsertPoint(end_);
        Value* else_block_v = else_block->codegen();
        if (!else_block_v) return nullptr;

        TheFunction->getBasicBlockList().push_back(end_);
        end_ = BasicBlock::Create(TheContext, "branchend");
    }

    TheFunction->getBasicBlockList().push_back(end_);
    Builder.CreateBr(end_);
    Builder.SetInsertPoint(end_);

    return cond;
};

Value* WhileStmt::codegen() {
    Function* TheFunction = Builder.GetInsertBlock()->getParent();
    BasicBlock* cond_check = BasicBlock::Create(TheContext, "cond_check", TheFunction);
    BasicBlock* block_ = BasicBlock::Create(TheContext, "loop_block");
    BasicBlock* end_ = BasicBlock::Create(TheContext, "loop_end");

    // cond_check block calculates the condition and then conditional branches to the code block or exits
    Builder.SetInsertPoint(cond_check);
    Value* cond = expr->codegen();
    Value* comp = Builder.CreateICmpNE(cond, ConstantInt::get(TheContext, APInt(1,1)), "loop_cond");
    Builder.CreateCondBr(comp, block_, end_);

    // Code block executes the body of the while loop
    TheFunction->getBasicBlockList().push_back(block_);
    Builder.SetInsertPoint(block_);
    Value* block_v = block->codegen();
    if (!block_v) return nullptr;
    Builder.CreateBr(cond_check);

    // The end of the loop
    TheFunction->getBasicBlockList().push_back(end_);
    Builder.SetInsertPoint(end_);
    
    return cond;
};
