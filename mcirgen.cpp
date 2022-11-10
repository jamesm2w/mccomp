#include "mccomp.hh"

using namespace llvm;
using namespace llvm::sys;

template<typename... Args>
Value* ThrowValueError( const std::string& format, Args... args ) {

    int size_s = std::snprintf( nullptr, 0, format.c_str(), args... ) + 1; // Extra space for '\0'
    if( size_s <= 0 ){ fprintf(stderr, "Error in formating error message"); return nullptr; }
    auto size = static_cast<size_t>( size_s );
    std::unique_ptr<char[]> buf( new char[ size ] );
    std::snprintf( buf.get(), size, format.c_str(), args... );

    fprintf(stderr, "Codegen Error ");
    fprintf(stderr, "%s", std::string( buf.get(), buf.get() + size - 1 ).c_str()); // We don't want the '\0' inside
    fprintf(stderr, "\n");

    return nullptr;
}

// forces alloca to be declared in the function entry point
AllocaInst* createEntryBlockAlloca(Function* TheFunction, Type* type, const std::string& varName) {
    IRBuilder<> TmpBuilder(&TheFunction->getEntryBlock(), TheFunction->getEntryBlock().begin());
    auto alloca = TmpBuilder.CreateAlloca(type, 0, varName.c_str());
    
    Value* ex = getDefaultValue(type);
    if (!ex) return nullptr;

    // Value* ex;
    // if (type->isIntegerTy(1)) {
    //     ex = ConstantInt::get(getBooleanType(), APInt(1, 0, false));
    // } else if (type->isIntegerTy(32)) {
    //     ex = ConstantInt::get(getIntegerType(), APInt(32, 0, true));
    // } else if (type->isFloatTy()) {
    //     ex = ConstantFP::get(TheContext, APFloat((float)0.0));
    // } else {
    //     ThrowValueError("Can't creat an initialiser for an alloca (unknown type)");
    //     return nullptr;
    // }
    TmpBuilder.CreateStore(ex, alloca);

    return alloca;
}

Value* Noop::codegen() {
    return 0;
}

Value* BoolLit::codegen() {
    printf("Boolean Literal... %d\n", val);
    if (val) {
        return ConstantInt::get(TheContext, APInt(1, 1, false));
    } else {
        return ConstantInt::get(TheContext, APInt(1, 0, false));
    }
}

Value* FloatLit::codegen() {
    printf("Floating Point Literal... %f\n", val);
    return ConstantFP::get(TheContext, APFloat((float)val));
}

Value* IntLit::codegen() {
    printf("Integer Literal... %d\n", val);
    return ConstantInt::get(TheContext, APInt(32, val, true));
}

Value* VariableLit::codegen() {
    printf("Generating variable literal... ");
    
    AllocaInst *V = NamedValues[tok.lexeme];
    if (!V) {
        GlobalVariable* GV = (GlobalVariable*) GlobalNamedValues[tok.lexeme];
        if (!GV) {
            return ThrowValueError("Couldn't find global variable with name \"%s\". Line %d Column %d.", tok.lexeme.c_str(), tok.lineNo, tok.columnNo); 
        }
        printf("Global Value Type %d\n", GV->getValueType()->getTypeID());
        return Builder.CreateLoad(GV->getValueType(), GV, "load_gvar");
    }

    printf("Value Type %d\n", V->getAllocatedType()->getTypeID());    
    return Builder.CreateLoad(V->getAllocatedType(), V, "load_lvar");
}

Value* FunctionCallLiteral::codegen() {
    printf("Function Call Literal...\n");
    Function* callee = TheModule->getFunction(ident.lexeme);

    if (!callee) return ThrowValueError("Undefined function \"%s\"", ident.lexeme.c_str());
    if (callee->arg_size() != args.size()) return ThrowValueError("Length of function parameters and supplied arguments is different");

    std::vector<Value*> argsv;
    for (uint i = 0, e = args.size(); i != e; ++i) {
        // TODO: Type Checking on Arguments to Functions
        argsv.push_back(args[i]->codegen());
        if (!argsv.back()) return nullptr;
    }

    if (callee->getReturnType()->isVoidTy()) {
        return Builder.CreateCall(callee, argsv);        
    }
    return Builder.CreateCall(callee, argsv, "call_func");
}

Value* UnaryOp::codegen() {
    printf("unary...\n");
    Value* operand = expr->codegen();
    if (!operand) return nullptr;

    switch (op.type) {
        case MINUS:
            if (isFloatingType(operand)) {
                return Builder.CreateFNeg(operand, "float_unary_sub");
            } else if (isIntegerType(operand)) {
                return Builder.CreateNeg(operand, "int_unary_sub");
            } else if (isBooleanType(operand)) {
                return Builder.CreateNeg(operand, "bool_unary_sub");
            } else {
                return ThrowValueError("Operand was not float, int or bool. What type is %d??", operand->getType()->getTypeID());
            }
        case NOT:
            operand = convertToBool(operand);
            if (!operand) return nullptr;
            return Builder.CreateNot(operand, "unary_not");
            // return Builder.CreateNeg(operand, "bool_unary_not");
        default:
            return ThrowValueError("Invalid Unary Operator \"%s\" expected \"!\", \"-\"", op.lexeme.c_str());
    }
}

Value* BinaryOp::codegen() {
    printf("binary...\n");
    Function* TheFunction = Builder.GetInsertBlock()->getParent();
    Value* left = lexpr->codegen();
    printf("op is %s\n", op.lexeme.c_str());
    Value* right = rexpr->codegen();
    if (!left || !right) return nullptr;

    switch (op.type) { // TODO: Switch this to lazy evaluation
        case AND:
            left = convertToBool(left);
            right = convertToBool(right);
            if (!left || !right) return nullptr;
            return Builder.CreateLogicalAnd(left, right);
        case OR:
            left = convertToBool(left);
            right = convertToBool(right);
            if (!left || !right) return nullptr;
            return Builder.CreateLogicalAnd(left, right);
    }

    std::pair<Value*, Value*> lr = convertToWider(left, right);
    if (!lr.first || !lr.second) return nullptr;

    left = lr.first;
    right = lr.second;

    if (isFloatingType(left)) {
        switch (op.type) {
            case PLUS:
                return Builder.CreateFAdd(left, right, "float_add");
            case MINUS:
                return Builder.CreateFSub(left, right, "float_sub");
            case ASTERIX:
                return Builder.CreateFMul(left, right, "float_mul");
            case DIV:
                return Builder.CreateFDiv(left, right, "float_div");
            case MOD:
                return Builder.CreateFRem(left, right, "float_mod");
            case LT:
                return Builder.CreateFCmpULT(left, right, "float_lt");
            case LE:
                return Builder.CreateFCmpULE(left, right, "float_le");
            case GT:
                return Builder.CreateFCmpUGT(left, right, "float_gt");
            case GE:
                return Builder.CreateFCmpUGE(left, right, "float_ge");
            case EQ:
                return Builder.CreateFCmpUEQ(left, right, "float_eq");
            case NE:
                return Builder.CreateFCmpUNE(left, right, "float_ne");
            default:
                return ThrowValueError("Unknown operator \"%s\"", op.lexeme.c_str());
        }
    } else if (isIntegerType(left)) {
        switch (op.type) {
            case PLUS:
                return Builder.CreateAdd(left, right, "int_add");
            case MINUS:
                return Builder.CreateSub(left, right, "int_sub");
            case ASTERIX:
                return Builder.CreateMul(left, right, "int_mul");
            case DIV:
                return Builder.CreateExactSDiv(left, right, "int_div_exact");
            case MOD:
                return Builder.CreateSRem(left, right, "int_mod");
            case LT:
                return Builder.CreateICmpSLT(left, right, "int_lt");
            case LE:
                return Builder.CreateICmpSLE(left, right, "int_le");
            case GT:
                return Builder.CreateICmpSGT(left, right, "int_gt");
            case GE:
                return Builder.CreateICmpSGE(left, right, "int_ge");
            case EQ:
                return Builder.CreateICmpEQ(left, right, "int_eq");
            case NE:
                return Builder.CreateICmpNE(left, right, "int_ne");
            default:
                return ThrowValueError("Unknown operator \"%s\"", op.lexeme.c_str());
        }
    }

    return ThrowValueError("Casting returned something that was not a valid type. %d. line=%d col=%d", lr.first->getType()->getTypeID(), op.lineNo, op.columnNo);
}

Value* AssignExpr::codegen() {
    printf("assign...\n");
    Value* ex = expr->codegen();
    // TODO: typechecking on AssignExpr
    if (!ex) {
        return nullptr;
    }

    AllocaInst* alloca = NamedValues[ident.lexeme];
    if (!alloca) {
        
        Value* GV = GlobalNamedValues[ident.lexeme];
        if (!GV) {
            return ThrowValueError("Unknown variable name \"%s\"", ident.lexeme.c_str()); 
        }
        // GlobalVariable* gvar = (GlobalVariable *) GV; //TheModule->getNamedGlobal(tok.lexeme);
        // if (!gvar) return ThrowValueError("Couldn't find global variable \"%s\" in module table", ident.lexeme.c_str());
        
        return Builder.CreateStore(ex, GV);
    }

    return Builder.CreateStore(ex, alloca);
};

Value* LocalDecl::codegen() {
    printf("local...\n");
    NamedValues[ident.lexeme] = createEntryBlockAlloca(Builder.GetInsertBlock()->getParent(), getTokType(type), ident.lexeme);
    if (!NamedValues[ident.lexeme]) return nullptr;
    return NamedValues[ident.lexeme];
};

Value* Block::codegen() { // TODO: Scoping for blocks
    printf("block...\n");
    Function* TheFunction = Builder.GetInsertBlock()->getParent();
    BasicBlock* block = BasicBlock::Create(TheContext, "start_block", TheFunction);
    BasicBlock* end = BasicBlock::Create(TheContext, "end_block");
    
    Builder.CreateBr(block);
    Builder.SetInsertPoint(block);

    for (auto & decl : local_decls) {
        if (!decl->codegen()) return nullptr;
    }

    for (auto & stmt : stmt_list) {
        if (!stmt->codegen()) return nullptr;
    }

    // return block;
    // TODO: handle end of blocks
    TheFunction->getBasicBlockList().push_back(end);
    Value* br = Builder.CreateBr(end);
    Builder.SetInsertPoint(end);
    return block;
    // return br;
};

// Only used syntactically. No need to generate code for the param.
Value* Param::codegen() {
    printf("param...\n");
    return nullptr;
};

Value* prototype_codegen(TOKEN type, TOKEN ident, vector<unique_ptr<Param>>& params) {
    printf("prototype...\n");
    uint idx = 0;
    std::vector<Type*> param_list(params.size());
    for (auto &param : params) {
        Type* type = getTokType(param->get_type());
        if (!type) return ThrowValueError("Invalid Type \"%s\"", param->get_type().lexeme.c_str());
        param_list[idx++] = type;
    }

    Type* return_type = getTokType(type);
    if (!return_type) return ThrowValueError("Invalid Type \"%s\"", type.lexeme.c_str());

    FunctionType* FT = FunctionType::get(return_type, param_list, false);
    Function* F = Function::Create(FT, Function::ExternalLinkage, ident.lexeme, TheModule.get());
    unsigned int size = F->arg_size();
    idx = 0;
    
    for (Argument &arg : F->args()) {
        arg.setName(params[idx++]->get_name().lexeme);
    }

    return F;
}

Value* ExternDecl::codegen() {
    printf("extern...\n");
    return prototype_codegen(type, ident, params);
}

Value* GlobalDecl::codegen() {

    if (block) {
        printf("function...\n");
        // Function Declaration
        Function* TheFunction = TheModule->getFunction(ident.lexeme);

        if (!TheFunction) 
            TheFunction = (Function*) prototype_codegen(type, ident, params);
        
        if (!TheFunction) return nullptr;

        BasicBlock* BB = BasicBlock::Create(TheContext, "entry", TheFunction);
        Builder.SetInsertPoint(BB);

        NamedValues.clear();
        for (auto &arg : TheFunction->args()) {
            printf("arg for arging %s\n", arg.getName());
            AllocaInst *alloca = createEntryBlockAlloca(TheFunction, arg.getType(), std::string(arg.getName()));
            if (!alloca) return nullptr;
            Builder.CreateStore(&arg, alloca);
            NamedValues[std::string(arg.getName())] = alloca;

        }

        printf("block...\n");
        Value* block_v = block->codegen();
        if (!block_v) {
            TheFunction->eraseFromParent();
            return nullptr;
        }

        // just create a default return at the end of the function
        printf("function return type id %d", TheFunction->getReturnType()->getTypeID());
        Builder.CreateRet(getDefaultValue(TheFunction->getReturnType()));

        printf("verifying...\n");
        // Apparently should create a ret here but that should be managed by the code?
        verifyFunction(*TheFunction);
        return TheFunction;

    } else {
        printf("global...\n");
        // Global Variable Declaration
        if (GlobalNamedValues[ident.lexeme]) {
            return ThrowValueError("Global Variable \"%s\" is already defined", ident.lexeme.c_str());
        }
        
        Type* typ = getTokType(type);
        GlobalVariable* glob = new GlobalVariable(typ, false, GlobalValue::ExternalLinkage);
        Constant* defaultInit = getDefaultValue(typ);

        if (!defaultInit) return nullptr;
        glob->setInitializer(defaultInit);

        TheModule->getGlobalList().push_back(glob);
        
        // Value* val = TheModule->getOrInsertGlobal(ident.lexeme.c_str(), typ);
        GlobalNamedValues[ident.lexeme] = glob;
        printf("%s\n", GlobalNamedValues[ident.lexeme]);
        return glob;
    } 
}

Value* Program::codegen() {
    printf("program...\n");
    for (auto & extern_decl : extern_list) {
        if (!extern_decl->codegen()) return nullptr;
    }

    for (auto & decl : decl_list) {
        if (!decl->codegen()) return nullptr;
    }

    return Builder.GetInsertBlock();
};

Value* ReturnStmt::codegen() {
    printf("return stmt...\n");
    if (expr.has_value()) {
        Value* val = expr->get()->codegen();
        if (!val) return nullptr;
        return Builder.CreateRet(val);
    } else {
        return Builder.CreateRetVoid();
    }
}

Value* IfStmt::codegen() {
    printf("ifstmt...\n");
    Function* TheFunction = Builder.GetInsertBlock()->getParent();
    BasicBlock* true_ = BasicBlock::Create(TheContext, "true_branch", TheFunction);
    
    BasicBlock* end_;
    if (else_block)
        end_ = BasicBlock::Create(TheContext, "else_branch");
    else 
        end_ = BasicBlock::Create(TheContext, "end_branch");
    
    Value* cond = expr->codegen();
    if (!cond) return nullptr;

    cond = convertToBool(cond);
    if (!cond) return nullptr;    

    // Not Equal to 0 => True
    Value* comp = Builder.CreateICmpNE(cond, ConstantInt::get(TheContext, APInt(1, 0, false)), "cond_branch");

    Builder.CreateCondBr(comp, true_, end_);
    Builder.SetInsertPoint(true_);

    Value* true_block_v = block->codegen();
    if (!true_block_v) return nullptr;

    if (else_block) {
        BasicBlock* end__ = BasicBlock::Create(TheContext, "end_branch"); // create the end-of-branch block
        Builder.CreateBr(end__); // jump from true block to end block

        TheFunction->getBasicBlockList().push_back(end_);
        Builder.SetInsertPoint(end_);
        Value* else_block_v = else_block->codegen();
        if (!else_block_v) return nullptr;

        // TheFunction->getBasicBlockList().push_back(end_); // push back the else block
        end_ = end__; // set the end block to the end of the else stmt block
    }

    TheFunction->getBasicBlockList().push_back(end_); // add the end block to the program
    Builder.CreateBr(end_); // put a branch from the last block (else block if else or true block if no else) to the end
    Builder.SetInsertPoint(end_); // write code in this block going forwards

    return cond;
};

Value* WhileStmt::codegen() {
    printf("while...\n");
    Function* TheFunction = Builder.GetInsertBlock()->getParent();
    BasicBlock* cond_check = BasicBlock::Create(TheContext, "cond_check", TheFunction);
    BasicBlock* block_ = BasicBlock::Create(TheContext, "loop_block");
    BasicBlock* end_ = BasicBlock::Create(TheContext, "loop_end");

    // cond_check block calculates the condition and then conditional branches to the code block or exits
    Builder.CreateBr(cond_check);
    Builder.SetInsertPoint(cond_check);
    Value* cond = expr->codegen();
    if (!cond) return nullptr;
    // Not equal to 0 => True
    Value* comp = Builder.CreateICmpNE(cond, ConstantInt::get(TheContext, APInt(1, 0, false)), "loop_cond");
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
