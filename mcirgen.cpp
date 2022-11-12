#include "mccomp.hh"

using namespace llvm;
using namespace llvm::sys;

AllocaInst *Scope::searchForValue(std::string string) {
    if (values[string]) {
        return values[string];
    } else if (super) {
        return super->searchForValue(string);
    } else {
        return nullptr;
    }
}

bool Scope::insertValue(std::string name, llvm::AllocaInst *val) {
    if (values[name]) {
        return false;
    } else {
        values[name] = val;
        return true;
    }
}

Scope Scope::createSubScope() {
    return new Scope(this);
}

Scope scope_ptr;

template <typename... Args>
Value *ThrowValueError(const std::string &format, Args... args) {
    int size_s = std::snprintf(nullptr, 0, format.c_str(), args...) + 1;  // Extra space for '\0'
    if (size_s <= 0) {
        fprintf(stderr, "Error in formating error message");
        return nullptr;
    }
    auto size = static_cast<size_t>(size_s);
    std::unique_ptr<char[]> buf(new char[size]);
    std::snprintf(buf.get(), size, format.c_str(), args...);

    fprintf(stderr, "Codegen Error ");
    fprintf(stderr, "%s", std::string(buf.get(), buf.get() + size - 1).c_str());  // We don't want the '\0' inside
    fprintf(stderr, "\n");

    return nullptr;
}

// forces alloca to be declared in the function entry point
AllocaInst *createEntryBlockAlloca(Function *TheFunction, Type *type, const std::string &varName) {
    IRBuilder<> TmpBuilder(&TheFunction->getEntryBlock(), TheFunction->getEntryBlock().begin());
    auto alloca = TmpBuilder.CreateAlloca(type, 0, varName.c_str());

    Value *ex = getDefaultValue(type);
    if (!ex)
        return nullptr;

    TmpBuilder.CreateStore(ex, alloca);

    return alloca;
}

// Blank AST Node which doesn't do any codegen
Value *Noop::codegen() {
    return nullptr;
}

// Return a boolean literal
Value *BoolLit::codegen() {
    if (val) {
        return ConstantInt::get(TheContext, APInt(1, 1, false));
    } else {
        return ConstantInt::get(TheContext, APInt(1, 0, false));
    }
}

// Return a float literal
Value *FloatLit::codegen() {
    return ConstantFP::get(TheContext, APFloat((float)val));
}

// Return an integer literal
Value *IntLit::codegen() {
    return ConstantInt::get(TheContext, APInt(32, val, true));
}

// Load a named variable from the scope
Value *VariableLit::codegen() {
    AllocaInst *V = scope_ptr.searchForValue(tok.lexeme);
    if (!V) {
        GlobalVariable *GV = (GlobalVariable *)GlobalNamedValues[tok.lexeme];
        if (!GV) {
            return ThrowValueError("On Line %d Column %d: Undefined identifier \"%s\".", tok.lineNo, tok.columnNo, tok.lexeme.c_str());
        }

        return Builder.CreateLoad(GV->getValueType(), GV, "load_gvar");
    }

    return Builder.CreateLoad(V->getAllocatedType(), V, "load_lvar");
}

// Call a named function
Value *FunctionCallLiteral::codegen() {
    Function *callee = TheModule->getFunction(ident.lexeme);

    if (!callee)
        return ThrowValueError("On Line %d Column %d: No function with identifier \"%s\" found.", ident.lineNo, ident.columnNo, ident.lexeme.c_str());
    if (callee->arg_size() != args.size())
        return ThrowValueError("On Line %d Column %d: Length of defined function parameters and supplied arguments is different", ident.lineNo, ident.columnNo);

    std::vector<Value *> argsv;
    for (uint i = 0, e = args.size(); i != e; ++i) {
        // TODO: Type Checking on Arguments to Functions
		Value* argv = args[i]->codegen();
		if (callee->getArg(i)->getType()->isIntegerTy(1)) {
			argv = convertToBool(argv);
		} else if (callee->getArg(i)->getType()->isIntegerTy(32)) {
			argv = convertToInteger(argv);
		} else if (callee->getArg(i)->getType()->isFloatTy()) {
			argv = convertToFloat(argv);
		}
		if (!argv) return nullptr;

        argsv.push_back(argv);
        if (!argsv.back())
            return nullptr;
    }

    if (callee->getReturnType()->isVoidTy()) {
        return Builder.CreateCall(callee, argsv);
    }
    return Builder.CreateCall(callee, argsv, "call_func");
}

// Perform a unary operation
Value *UnaryOp::codegen() {
    Value *operand = expr->codegen();
    if (!operand)
        return nullptr;

    switch (op.type) {
        case MINUS:
            if (isFloatingType(operand)) {
                return Builder.CreateFNeg(operand, "float_unary_sub");
            } else if (isIntegerType(operand)) {
                return Builder.CreateNeg(operand, "int_unary_sub");
            } else if (isBooleanType(operand)) {
                return Builder.CreateNeg(operand, "bool_unary_sub");
            } else {
                return ThrowValueError("On Line %d Column %d: Operand was invalid type. Expected a float, int or bool, got TypID: %d??", op.lineNo, op.columnNo, operand->getType()->getTypeID());
            }
        case NOT:
            operand = convertToBool(operand);
            if (!operand)
                return nullptr;

            return Builder.CreateNot(operand, "unary_not");
        default:
            return ThrowValueError("On Line %d Column %d: Invalid Unary Operator \"%s\" expected \"!\", \"-\"", op.lineNo, op.columnNo, op.lexeme.c_str());
    }
}

// Perform a boolean lazy evaluation of "||" or "&&"
Value *booleanLazyEvaluation(std::unique_ptr<ASTNode> left, TOKEN op, std::unique_ptr<ASTNode> right) {
    Function *TheFunction = Builder.GetInsertBlock()->getParent();
    BasicBlock *LB = BasicBlock::Create(TheContext, "lazy_left_eval", TheFunction);
    BasicBlock *RB = BasicBlock::Create(TheContext, "lazy_right_eval");
    BasicBlock *end_ = BasicBlock::Create(TheContext, "lazy_end");
    // Jump in and eval left first
    Builder.CreateBr(LB);
    Builder.SetInsertPoint(LB);
    Value *lval = left->codegen();
    lval = convertToBool(lval);
    if (!lval)
        return nullptr;

    // if left == true|false then branch to eval right or to end (the LAZY part)
    if (op.type == AND) {
        Builder.CreateCondBr(lval, RB, end_);
    } else if (op.type == OR) {
        Builder.CreateCondBr(lval, end_, RB);
    } else {
        return nullptr;
    }

    // Generate eval right expr
    TheFunction->getBasicBlockList().push_back(RB);
    Builder.SetInsertPoint(RB);
    Value *rval = right->codegen();
    rval = convertToBool(rval);
    if (!rval)
        return nullptr;
    Builder.CreateBr(end_);

    TheFunction->getBasicBlockList().push_back(end_);
    Builder.SetInsertPoint(end_);

    // Using a PHI node we can return the left value or the right value depending on which block we executed
    PHINode *phi = Builder.CreatePHI(getBooleanType(), 2, "lazy_value");
    phi->addIncoming(lval, LB);
    phi->addIncoming(rval, RB);

    return phi;
}

// Compare two values LT, LE, etc.
Value *binaryCmpOperator(std::unique_ptr<ASTNode> lexpr, TOKEN op, std::unique_ptr<ASTNode> rexpr) {
    CmpInst::Predicate pred = CmpInst::FCMP_FALSE;
    std::pair<Value *, Value *> lr = convertToWider(lexpr->codegen(), rexpr->codegen());
    Value *left = lr.first;
    Value *right = lr.second;
    if (!left || !right)
        return nullptr;

    // Check the conversion returned valid types in the program
    bool floating = isFloatingType(left);
    bool integer = isIntegerType(left);
    bool boolean = isBooleanType(left);
    if (!floating && !integer && !boolean) {
        return ThrowValueError("On Line %d Column %d: Type of expressions was not valid.", op.lineNo, op.columnNo);
    }

    // Calculate the Cmp Instance enum for the operator
    switch (op.type) {
        case LT:
            if (floating)
                pred = CmpInst::FCMP_ULT;
            if (integer)
                pred = CmpInst::ICMP_SLT;
            if (boolean)
                pred = CmpInst::ICMP_ULT;
            break;
        case LE:
            if (floating)
                pred = CmpInst::FCMP_ULE;
            if (integer)
                pred = CmpInst::ICMP_SLE;
            if (boolean)
                pred = CmpInst::ICMP_ULE;
            break;
        case GT:
            if (floating)
                pred = CmpInst::FCMP_UGT;
            if (integer)
                pred = CmpInst::ICMP_SGT;
            if (boolean)
                pred = CmpInst::ICMP_UGT;
            break;
        case GE:
            if (floating)
                pred = CmpInst::FCMP_UGE;
            if (integer)
                pred = CmpInst::ICMP_SGE;
            if (boolean)
                pred = CmpInst::ICMP_UGE;
            break;
        case EQ:
            if (floating)
                pred = CmpInst::FCMP_UEQ;
            if (integer || boolean)
                pred = CmpInst::ICMP_EQ;
            break;
        case NE:
            if (floating)
                pred = CmpInst::FCMP_UNE;
            if (integer || boolean)
                pred = CmpInst::ICMP_NE;
            break;
        default:  // Not a comparison operator
            return ThrowValueError("On Line %d Column %d: Operator was not a comparison operator", op.lineNo, op.columnNo);
    }

    if (pred != CmpInst::FCMP_FALSE) {
        return Builder.CreateCmp(pred, left, right, "bin_cmp_op");
    } else {
        return ThrowValueError("On Line %d Column %d: Operator and types were not valid.", op.lineNo, op.columnNo);
    }
}

// Execute binary operation
Value *BinaryOp::codegen() {
    // Do special lazy eval for boolean ops || and &&
    if (op.type == AND || op.type == OR)
        return booleanLazyEvaluation(std::move(lexpr), op, std::move(rexpr));

    if (op.type == LT || op.type == LE || op.type == GT || op.type == LE || op.type == GE || op.type == EQ || op.type == NE) {
        return binaryCmpOperator(std::move(lexpr), op, std::move(rexpr));
    }

    Function *TheFunction = Builder.GetInsertBlock()->getParent();

    std::pair<Value *, Value *> lr = convertToWider(lexpr->codegen(), rexpr->codegen());
    Value *left = lr.first;
    Value *right = lr.second;
    if (!left || !right)
        return nullptr;

    // Boolean values don't work with arithmetic expressions like +, - so upcast them instead?
    if (isBooleanType(left)) {
        left = boolToInteger(left);
        right = boolToInteger(right);
        if (!left || !right)
            return nullptr;
    }

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
            default:
                return ThrowValueError("On Line %d Column %d: Invalid Operator \"%s\"", op.lineNo, op.columnNo, op.lexeme.c_str());
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
            default:
                return ThrowValueError("On Line %d Column %d: Invalid Operator \"%s\"", op.lineNo, op.columnNo, op.lexeme.c_str());
        }
    }

    return ThrowValueError("On Line %d Column %d: Casting returned something that was not a valid type with TypeID: %d", op.lineNo, op.columnNo, lr.first->getType()->getTypeID());
}

// Generate a store for the local/global variable with an expression
Value *AssignExpr::codegen() {
    Value *ex = expr->codegen();
    // TODO: typechecking on AssignExpr
    if (!ex)
        return nullptr;

    // Lookup the variable name to assign for in the scope symbol table
    AllocaInst *alloca = scope_ptr.searchForValue(ident.lexeme);
    if (!alloca) {
        // If not found in the symbol table then try the global variable map
        Value *GV = GlobalNamedValues[ident.lexeme];
        if (!GV) {
            // If not a local or global not a defined identifier
            return ThrowValueError("On Line %d Column %d: Unknown identifier \"%s\".", ident.lineNo, ident.columnNo, ident.lexeme.c_str());
        }

        return Builder.CreateStore(ex, GV);
    }

    return Builder.CreateStore(ex, alloca);
};

// Define a local variable in the current function/block
Value *LocalDecl::codegen() {
    // TODO: Test local decls on sub-blocks and how that interacts with scope
    // Create the alloca instance at the entry block to the function
    AllocaInst *alloca = createEntryBlockAlloca(Builder.GetInsertBlock()->getParent(), getTokType(type), ident.lexeme);

    // Try insert the alloca in the symbol table. If it fails raise an error.
    if (!scope_ptr.insertValue(ident.lexeme, alloca)) {
        return ThrowValueError("On Line %d Column %d: Identifier \"%s\" is already declared in this scope.", ident.lineNo, ident.columnNo, ident.lexeme.c_str());
    }

    return alloca;
};

// Create some basic blocks and a subscope
Value *Block::codegen() {
    Function *TheFunction = Builder.GetInsertBlock()->getParent();
    BasicBlock *block = BasicBlock::Create(TheContext, "start_block", TheFunction);
    BasicBlock *end = BasicBlock::Create(TheContext, "end_block");

    Builder.CreateBr(block);
    Builder.SetInsertPoint(block);

    // Create new scope
    Scope prev_scope_ptr = scope_ptr;
    scope_ptr = new Scope(scope_ptr);

    for (auto &decl : local_decls) {
        if (!decl->codegen())
            return nullptr;
    }

    for (auto &stmt : stmt_list) {
        if (!stmt->codegen())
            return nullptr;
    }

    // Revert up to the upper scope
    scope_ptr = prev_scope_ptr;

    TheFunction->getBasicBlockList().push_back(end);
    Builder.CreateBr(end);
    Builder.SetInsertPoint(end);
    return block;
};

// Only used syntactically. No need to generate code for the param.
Value *Param::codegen() {
    return nullptr;
};

// Creates and emites a function prototype for extern or decl
Value *prototype_codegen(TOKEN type, TOKEN ident, vector<unique_ptr<Param>> &params) {
    uint idx = 0;
    std::vector<Type *> param_list(params.size());

    for (auto &param : params) {  // Take each param and add it to the type vector
        TOKEN param_t = param->get_type();
        Type *type = getTokType(param_t);

        if (!type)
            return ThrowValueError("On Line %d Column %d: Invalid Type \"%s\"", param_t.lineNo, param_t.columnNo, param_t.lexeme.c_str());
        param_list[idx++] = type;
    }

    // Get the return type of the function
    Type *return_type = getTokType(type);
    if (!return_type)
        return ThrowValueError("On Line %d Column %d: Invalid Type \"%s\"", type.lineNo, type.columnNo, type.lexeme.c_str());

    // Create the function types
    FunctionType *FT = FunctionType::get(return_type, param_list, false);
    Function *F = Function::Create(FT, Function::ExternalLinkage, ident.lexeme, TheModule.get());

    // Prepare the argument names
    idx = 0;
    for (Argument &arg : F->args()) {
        arg.setName(params[idx++]->get_name().lexeme);
    }

    return F;
}

// External declarations are just an externally linked prototype
Value *ExternDecl::codegen() {
    return prototype_codegen(type, ident, params);
}

// Declares a global variable or a function
Value *GlobalDecl::codegen() {
    if (block) {
        // Function Declaration

        Function *TheFunction = TheModule->getFunction(ident.lexeme);  // Get function proto if already defined -- shouldn't happen
        if (!TheFunction)
            TheFunction = (Function *)prototype_codegen(type, ident, params);  // Else make one!
        if (!TheFunction)
            return nullptr;

        // Create the entry block for the function
        BasicBlock *BB = BasicBlock::Create(TheContext, "entry", TheFunction);
        Builder.SetInsertPoint(BB);

        // Create the function scope (params, etc) a new inner scope is created for the block
        Scope prev_scope_ptr = scope_ptr;
        scope_ptr = new Scope(prev_scope_ptr);

        // Loop through each arg and create the allocas for them. Then put them in the scope symbol table
        for (auto &arg : TheFunction->args()) {
            AllocaInst *alloca = createEntryBlockAlloca(TheFunction, arg.getType(), std::string(arg.getName()));
            if (!alloca)
                return nullptr;

            Builder.CreateStore(&arg, alloca);
            scope_ptr.insertValue(std::string(arg.getName()), alloca);
        }

        // Create the actual function code
        Value *block_v = block->codegen();
        if (!block_v) {
            TheFunction->eraseFromParent();  // If a function is invalid
            return nullptr;
        }

        // Revert to the previous scope
        scope_ptr = prev_scope_ptr;

        // Create a default return at the end of the function to ensure validity if previous blocks did not create one
        Builder.CreateRet(getDefaultValue(TheFunction->getReturnType()));

        if (!verifyFunction(*TheFunction)) {  // Returns true if issue found in function. If so stop IR generation.
            TheFunction->eraseFromParent();
            return ThrowValueError("On Line %d Column %d: Function Creation returned invalid IR", type.lineNo, type.columnNo);
        }

        return TheFunction;
    } else {
        // Global Variable Declaration

        // Don't allow a global to be defined twice
        if (GlobalNamedValues[ident.lexeme]) {
            return ThrowValueError("On Line %d Column %d: Global Variable \"%s\" is already defined", ident.lineNo, ident.columnNo, ident.lexeme.c_str());
        }

        Type *typ = getTokType(type);
        GlobalVariable *glob = new GlobalVariable(typ, false, GlobalValue::ExternalLinkage);
        Constant *defaultInit = getDefaultValue(typ);
        if (!defaultInit)
            return nullptr;

        // Create initialiser for the value, and put it in the module/symbol table
        glob->setInitializer(defaultInit);
        TheModule->getGlobalList().push_back(glob);
        GlobalNamedValues[ident.lexeme] = glob;

        return glob;
    }
}

// Create the program
Value *Program::codegen() {
    // Just go through each part and codegen it.
    for (auto &extern_decl : extern_list) {
        if (!extern_decl->codegen())
            return nullptr;
    }

    for (auto &decl : decl_list) {
        if (!decl->codegen())
            return nullptr;
    }

    return Builder.GetInsertBlock();
};

// Create the return statement
Value *ReturnStmt::codegen() {
    // TODO: Type Checking on return statement
    if (expr.has_value()) {
        Value *val = expr->get()->codegen();
        if (!val)
            return nullptr;

        return Builder.CreateRet(val);
    } else {
        return Builder.CreateRetVoid();
    }
}

// If control statement
Value *IfStmt::codegen() {
    Function *TheFunction = Builder.GetInsertBlock()->getParent();

    // Create the branches
    BasicBlock *true_ = BasicBlock::Create(TheContext, "true_branch");
    BasicBlock *end_;
    if (else_block)  // If there's an else block we create that as the second block
        end_ = BasicBlock::Create(TheContext, "else_branch");
    else  // If no else block we can just create the end block
        end_ = BasicBlock::Create(TheContext, "end_branch");

    // Create the condition
    Value *cond = expr->codegen();
    cond = convertToBool(cond);
    if (!cond)
        return nullptr;

    // TODO: Remove comp here if we're already converting to bool;
    // Not Equal to 0 => True
    // Value* comp = Builder.CreateICmpNE(cond, ConstantInt::get(TheContext, APInt(1, 0, false)), "cond_branch");

    // The "IF" bit -- branch to true, else etc
    Builder.CreateCondBr(cond, true_, end_);
    TheFunction->getBasicBlockList().push_back(true_);
    Builder.SetInsertPoint(true_);

    // Generate the true block
    Value *true_block_v = block->codegen();
    if (!true_block_v)
        return nullptr;

    // Optionally generate the else block and create the end block
    if (else_block) {
        BasicBlock *end__ = BasicBlock::Create(TheContext, "end_branch");  // create the end-of-branch block
        Builder.CreateBr(end__);                                           // jump from true block to end block

        TheFunction->getBasicBlockList().push_back(end_);  // emit else block
        Builder.SetInsertPoint(end_);
        Value *else_block_v = else_block->codegen();  // write to else block
        if (!else_block_v)
            return nullptr;

        end_ = end__;  // set the end block to the end of the else stmt block
    }

    // Finish up with the end block
    TheFunction->getBasicBlockList().push_back(end_);  // add the end block to the program
    Builder.CreateBr(end_);                            // put a branch from the last block (else block if else or true block if no else) to the end
    Builder.SetInsertPoint(end_);                      // write code in this block going forwards

    return cond;
};

// While control statement
Value *WhileStmt::codegen() {
    // Create the necessary blocks
    Function *TheFunction = Builder.GetInsertBlock()->getParent();
    BasicBlock *cond_check = BasicBlock::Create(TheContext, "cond_check", TheFunction);
    BasicBlock *block_ = BasicBlock::Create(TheContext, "loop_block");
    BasicBlock *end_ = BasicBlock::Create(TheContext, "loop_end");

    // cond_check block calculates the condition and then conditional branches to the code block or exits
    Builder.CreateBr(cond_check);
    Builder.SetInsertPoint(cond_check);
    Value *cond = expr->codegen();
    if (!cond)
        return nullptr;

    // TODO: Bool conversion on while loop?
    // Not equal to 0 => True

    Value *comp = Builder.CreateICmpNE(cond, ConstantInt::get(TheContext, APInt(1, 0, false)), "loop_cond");
    Builder.CreateCondBr(comp, block_, end_);

    // Code block executes the body of the while loop
    TheFunction->getBasicBlockList().push_back(block_);
    Builder.SetInsertPoint(block_);
    Value *block_v = block->codegen();
    if (!block_v)
        return nullptr;
    Builder.CreateBr(cond_check);

    // The end of the loop
    TheFunction->getBasicBlockList().push_back(end_);
    Builder.SetInsertPoint(end_);

    return cond;
};
