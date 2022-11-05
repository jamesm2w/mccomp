#include "mccomp.hh"

//===----------------------------------------------------------------------===//
// Parser
//===----------------------------------------------------------------------===//

/// CurTok/getNextToken - Provide a simple token buffer.  CurTok is the current
/// token the parser is looking at.  getNextToken reads another token from the
/// lexer and updates CurTok with its results.

TOKEN getNextToken() {

  if (tok_buffer.size() == 0)
    tok_buffer.push_back(gettok());

  TOKEN temp = tok_buffer.front();
  tok_buffer.pop_front();

    // fprintf(stderr, "Token: %s with type %d\n", temp.lexeme.c_str(), temp.type);

  return CurTok = temp;
}

void putBackToken(TOKEN tok) { tok_buffer.push_front(tok); }

int getTokPrecedence() {
//     if (!isascii(CurTok.type))
//         return -1;
    
    int prec = BinopPrecedence[CurTok.type];
    if (prec <= 0) return -1;
    return prec;
}

//===----------------------------------------------------------------------===//
// Recursive Descent Parser - Function call for each production
//===----------------------------------------------------------------------===//

// Throw an error from a void function
template<typename ... Args>
void ThrowErrorV( const std::string& format, Args ... args ) {

    int size_s = std::snprintf( nullptr, 0, "%s", format.c_str(), args ... ) + 1; // Extra space for '\0'
    if( size_s <= 0 ){ fprintf(stderr, "Error in formating error message"); return; }
    auto size = static_cast<size_t>( size_s );
    std::unique_ptr<char[]> buf( new char[ size ] );
    std::snprintf( buf.get(), size, "%s", format.c_str(), args ... );

    fprintf(stderr, "Syntax Error on Line %d Col %d got \"%s\" ", CurTok.lineNo, CurTok.columnNo, CurTok.lexeme.c_str() );
    fprintf(stderr, "%s", std::string( buf.get(), buf.get() + size - 1 ).c_str()); // We don't want the '\0' inside
    fprintf(stderr, "\n");
}

// Throw an error in a function and return a null pointer.
template<class T, typename ... Args>
std::unique_ptr<T> ThrowError( const std::string &format, Args ... args) {
    ThrowErrorV(format, args ...);
    return nullptr;
}

// Print out the current token.
void DebugPrint() {
    fprintf(stderr, "Debug: CurTok[Type %d Line %d Col %d Lexeme \"%s\"]\n", CurTok.type, CurTok.lineNo, CurTok.columnNo, CurTok.lexeme.c_str());
}

void DebugPrint(std::string msg) {
    fprintf(stderr, "Debug: %s CurTok[Type %d Line %d Col %d Lexeme \"%s\"]\n", msg.c_str(), CurTok.type, CurTok.lineNo, CurTok.columnNo, CurTok.lexeme.c_str());
}

// Checks current token matches the given type and then gets the next one. Returns true if match. False otherwise.
// False matches don't get moved on from.
bool EatToken(int type) {
    if (CurTok.type == type) {
        getNextToken();
        return true;
    } else {
        return false;
    }
} 

/* Add function calls for each production */

// Program ::= ExternList DeclList
//   		 | DeclList
std::unique_ptr<Program> parser() {
    std::vector<std::unique_ptr<ExternDecl>> extern_list;
    std::vector<std::unique_ptr<GlobalDecl>> decl_list;
    // //DebugPrint();
    //DebugPrint("Parser Start");
    if (parse_extern_list(extern_list) < 0) {
        return nullptr;
    };
    
    // //DebugPrint();
    //DebugPrint("Externs finished. Starting Decls");
    if (parse_decl_list(decl_list) < 0) {
        return nullptr;
    }

    return std::make_unique<Program>(std::move(extern_list), std::move(decl_list));
}

// ExternList  ::= Extern ExternList'
// ExternList' ::= Extern ExternList'
//               | epsilon
int parse_extern_list(std::vector<std::unique_ptr<ExternDecl>> &list) {
    switch (CurTok.type) {
        case EXTERN: {
            //DebugPrint("Extern Item Detected");
            std::unique_ptr<ExternDecl> extern_item = parse_extern();
            if (extern_item == nullptr) {
                return -1;
            }
            list.push_back(std::move(extern_item));
            return parse_extern_list(list);
        }
        case VOID_TOK:
        case BOOL_TOK:
        case FLOAT_TOK:
        case INT_TOK:
            //DebugPrint("End of Externs Detected");
            return 0;
        default:
            //DebugPrint("Unknown Encountered");
            ThrowErrorV("Expected one of \"extern\", \"void\", \"bool\", \"float\", \"int\"");
            return -1;
    }
}

// Extern ::= extern TypeSpec IDENT ( Params ) ;
std::unique_ptr<ExternDecl> parse_extern() {
    
    if (!EatToken(EXTERN)) {
        return ThrowError<ExternDecl>("Expected \"extern\"");
    }

    TOKEN type = CurTok;
    getNextToken();
    
    TOKEN ident = CurTok;
    getNextToken();
    
    if (!EatToken(LPAR)) {
        return ThrowError<ExternDecl>("Expected \"(\"");
    }
    
    std::optional<std::vector<std::unique_ptr<Param>>> params = parse_params();
    if (!params.has_value()) {
        return nullptr;
    }
    
    if (!EatToken(RPAR)) {
        return ThrowError<ExternDecl>("Expected \")\"");
    }
    
    if (!EatToken(SC)) {
        return ThrowError<ExternDecl>("Expected \";\"");
    }
    
    return std::make_unique<ExternDecl>(type, ident, std::move(params.value()));
}

// DeclList  ::= Decl DeclList'
// DeclList' ::= Decl DeclList'
//             | epsilon
int parse_decl_list(std::vector<std::unique_ptr<GlobalDecl>> &list) {

    switch (CurTok.type) {
        case VOID_TOK:
        case BOOL_TOK:
        case FLOAT_TOK:
        case INT_TOK: {
            //DebugPrint("Decl Item Detected");
            std::unique_ptr<GlobalDecl> decl_item = parse_decl();
            if (decl_item == nullptr) {
                return -1;
            }
            list.push_back(std::move(decl_item));
            return parse_decl_list(list);
        }
        case EOF_TOK:
            //DebugPrint("EOF Detected");
            if (list.size() >= 1) { // Need at least one decl 
                return 0;
            }
        default:
            ThrowErrorV("Expected at least one variable or function declaration");
            return -1;
    }
}

// Decl ::= VarType IDENT Decl'
//        | void IDENT FunDecl
// Decl' ::= ;
//         | FunDecl
// FunDecl ::= ( Params ) Block
std::unique_ptr<GlobalDecl> parse_decl() {

    TOKEN type = CurTok;
    getNextToken();
    TOKEN ident = CurTok;
    getNextToken();

    if (type.type == VOID_TOK || CurTok.type == LPAR) {
        if (!EatToken(LPAR)) {
            return ThrowError<GlobalDecl>("Expected \"(\"");
        }
        // Function Decl
        std::optional<std::vector<std::unique_ptr<Param>>> params = parse_params();
        if (!params.has_value()) {
            return nullptr;
        }
        
        if (!EatToken(RPAR)) {
            return ThrowError<GlobalDecl>("Expected \")\"");
        }

        auto block = parse_block();
        if (block == nullptr) {
            return nullptr;
        }

        return std::make_unique<GlobalDecl>(type, ident, std::move(params.value()), std::move(block));
    } else if (CurTok.type == SC) {

        if (!EatToken(SC)) {
            return ThrowError<GlobalDecl>("Expected \";\"");
        }

        return std::make_unique<GlobalDecl>(type, ident);
    } else {
        return ThrowError<GlobalDecl>("Expected \"(\" or \";\""); 
    }
}

//TODO: Error checking for params & rest of parser & ast printing

// Params ::= Paramlist
//          | void
// 		 | epsilon
std::optional<std::vector<std::unique_ptr<Param>>> parse_params() {
    std::vector<std::unique_ptr<Param>> list;

    switch (CurTok.type) {
        case VOID_TOK:
            //DebugPrint("Void params");
            getNextToken();
        case RPAR:
            //DebugPrint("End of params found");
            return std::optional{ std::move(list) };
        case BOOL_TOK:
        case FLOAT_TOK:
        case INT_TOK:
            //DebugPrint("Param Found");
            if (parse_param_list(list) < 0) {
                return std::nullopt;
            }
            return std::optional{ std::move(list) };
        default:
            return std::nullopt;
    }
}

// ParamList  ::= Param ParamList'
// ParamList' ::= , Param ParamList'
// 			 | epsilon
int parse_param_list(std::vector<std::unique_ptr<Param>> &list) {
    switch (CurTok.type) {
        case BOOL_TOK:
        case FLOAT_TOK:
        case INT_TOK:
            //DebugPrint("param found");
            if (list.size() == 0) {
                auto param_item = parse_param();
                if (param_item == nullptr) {
                    return -1;
                }
                list.push_back(std::move(param_item));
                return parse_param_list(list);
            } else {
                ThrowErrorV("Expected \",\" before more parameters");
                return -1;
            }
            break;
        case COMMA:
            //DebugPrint("Params continuing");
            if (list.size() >= 1) {
                getNextToken(); // Eat the comma
                auto param_item = parse_param();
                if (param_item == nullptr) {
                    return -1;
                }
                
                list.push_back(std::move(param_item));
                return parse_param_list(list);
            } else {
                ThrowErrorV("Expected \"bool\", \"float\", \"int\" in parameter declaration first");
                return -1;
            }
            break;
        case RPAR:
            //DebugPrint("end of params found");
            return 0;
        default:
            return -1;
    }
}

// Param ::= VarType IDENT
std::unique_ptr<Param> parse_param() {
    TOKEN type = CurTok;
    getNextToken();
    TOKEN ident = CurTok;
    getNextToken();
    return std::make_unique<Param>(type, ident);
}

std::unique_ptr<Block> parse_block() {
    std::vector<std::unique_ptr<LocalDecl>> local_decls;
    std::vector<std::unique_ptr<ASTNode>> stmt_list;

    //DebugPrint("start block");

    if (!EatToken(LBRA)) {
        return ThrowError<Block>("Expected \"{\" at start of block");
    }

    if (parse_local_decls(local_decls) < 0) {
        return nullptr;
    }

    if (parse_stmt_list(stmt_list) < 0) {
        return nullptr;
    }

    if (!EatToken(RBRA)) {
        return ThrowError<Block>("Expected \"}\" at end of block");
    }

    return std::make_unique<Block>(std::move(local_decls), std::move(stmt_list));
}

// LocalDecls ::= LocalDecl LocalDecls
// 			 | epsilon
int parse_local_decls(std::vector<std::unique_ptr<LocalDecl>> &list) {
    switch (CurTok.type) {
        case BOOL_TOK:
        case FLOAT_TOK:
        case INT_TOK: {
            //DebugPrint("local decl");
            auto decl_item = parse_local_decl();
            if (decl_item == nullptr) {
                return -1;
            }
            list.push_back(std::move(decl_item));
            return parse_local_decls(list);
        }
        case BOOL_LIT:
        case FLOAT_LIT:
        case INT_LIT:
        case IDENT:
        case LPAR:
        case NOT:
        case MINUS:
        case RETURN:
        case IF:
        case WHILE:
        case SC:
        case RBRA:
        case LBRA:
            //DebugPrint("end of decl");
            return 0;
        default:
            ThrowErrorV("Expected declaration starting with \"bool\", \"float\", \"int\" or a statement.");
            return -1;
    }
}
// LocalDecl ::= VarType IDENT ;
std::unique_ptr<LocalDecl> parse_local_decl() {
    TOKEN type = CurTok;
    getNextToken();
    TOKEN ident = CurTok;
    getNextToken();
    if (!EatToken(SC)) {
        return ThrowError<LocalDecl>("Expected \";\" at the end of local declaration");
    }

    return std::make_unique<LocalDecl>(type, ident);
}

// StmtList ::= Stmt StmtList
// 		   | epsilon
int parse_stmt_list(std::vector<std::unique_ptr<ASTNode>> &list) {
    switch (CurTok.type) {
        case RBRA:
            //DebugPrint("end of stmt");
            return 0;
        case BOOL_LIT:
        case FLOAT_LIT:
        case INT_LIT:
        case IDENT:
        case LPAR:
        case NOT:
        case MINUS:
        case RETURN:
        case IF:
        case WHILE:
        case SC:
        case LBRA: {
            // DebugPrint("stmt found");
            auto stmt = parse_stmt();
            if (stmt == nullptr) {
                return -1;
            }
            list.push_back(std::move(stmt));
            return parse_stmt_list(list);
        }
        default:
            ThrowErrorV("Expected \"}\" end of block, or a new statement");
            return -1;
    }
}

// Stmt ::= ExprStmt
// 	   | Block
// 	   | IfStmt
// 	   | WhileStmt
// 	   | ReturnStmt
std::unique_ptr<ASTNode> parse_stmt() {
    switch (CurTok.type) {
        case LBRA:
            //DebugPrint("block");
            return parse_block();
        case WHILE:
            //DebugPrint("while");
            return parse_while_stmt();
        case IF:
            //DebugPrint("if");
            return parse_if_stmt();
        case RETURN:   
            // DebugPrint("return");
            return parse_return_stmt();
        case SC:
        case BOOL_LIT:
        case FLOAT_LIT:
        case INT_LIT:
        case IDENT:
        case LPAR:
        case NOT:
        case MINUS:
            // DebugPrint("expr");
            return parse_expr_stmt();
        default:
            return ThrowError<ASTNode>("Expected start of statement");
    }
}

// ExprStmt ::= Expr ;
// 		   | ;
std::unique_ptr<ASTNode> parse_expr_stmt() {
    switch (CurTok.type) {
        case SC:
            // DebugPrint("no expr");
            getNextToken(); // Eat semicolon
            return std::make_unique<Noop>();
        case BOOL_LIT:
        case FLOAT_LIT:
        case INT_LIT:
        case IDENT:
        case LPAR:
        case NOT:
        case MINUS: {
            //DebugPrint("expr expr");
            auto expr = parse_expr();
            if (expr == nullptr) {
                return nullptr;
            }
            if (!EatToken(SC)) {
                return ThrowError<ASTNode>("Expected \";\" at the end of a statement");
            }
            return expr;
        }
        default:
            return ThrowError<ASTNode>("Expected \";\" or start of expression");
    }
}

// WhileStmt ::= while ( Expr ) Stmt
std::unique_ptr<WhileStmt> parse_while_stmt() {
    if (!EatToken(WHILE)) {
        return ThrowError<WhileStmt>("Expected \"while\"");
    }
    if (!EatToken(LPAR)) {
        return ThrowError<WhileStmt>("Expected \"(\"");
    }
    auto expr = parse_expr();
    if (expr == nullptr) {
        return nullptr;
    }
    if (!EatToken(RPAR)) {
        return ThrowError<WhileStmt>("Expected \")\"");
    }
    auto stmt = parse_stmt();
    if (stmt == nullptr) {
        return nullptr;
    }
    return std::make_unique<WhileStmt>(std::move(expr), std::move(stmt));
}

// IfStmt ::= if ( Expr ) Block ElseStmt
// ElseStmt ::= else Block
//            | epsilon
std::unique_ptr<IfStmt> parse_if_stmt() {
    if (!EatToken(IF)) {
        return ThrowError<IfStmt>("Expected \"if\"");
    }
    if (!EatToken(LPAR)) {
        return ThrowError<IfStmt>("Expected \"(\"");
    }
    auto expr = parse_expr();
    if (expr == nullptr) {
        return nullptr;
    }
    if (!EatToken(RPAR)) {
        return ThrowError<IfStmt>("Expected \")\"");
    }
    auto block = parse_block();
    if (block == nullptr) {
        return nullptr;
    }
    switch (CurTok.type) {
        case ELSE: {
            getNextToken();
            auto e_block = parse_block();
            if (e_block == nullptr) {
                return nullptr;
            }
            return std::make_unique<IfStmt>(std::move(expr), std::move(block), std::move(e_block));
        }
        case BOOL_LIT:
        case FLOAT_LIT:
        case INT_LIT:
        case IDENT:
        case LPAR:
        case NOT:
        case MINUS:
        case RETURN:
        case IF:
        case WHILE:
        case SC:
        case LBRA:
        case RBRA:
            break;
        default:
            return ThrowError<IfStmt>("Expected \"else\" or next statement");
    }

    return std::make_unique<IfStmt>(std::move(expr), std::move(block), nullptr);
}

// ReturnStmt ::= return ExprStmt
std::unique_ptr<ReturnStmt> parse_return_stmt() {
    if (!EatToken(RETURN)) {
        return ThrowError<ReturnStmt>("Expected \"return\"");
    }

    if (CurTok.type == SC) {
        EatToken(SC);
        return std::make_unique<ReturnStmt>(std::nullopt);
    } else {
        auto expr = parse_expr_stmt();
        if (expr == nullptr) {
            return nullptr;
        }

        return std::make_unique<ReturnStmt>(std::move(expr));
    }
}

// Expr ::= IDENT = Expr
//        | Logical
std::unique_ptr<ASTNode> parse_expr() {
    if (CurTok.type == IDENT) {
        //DebugPrint("ident!!");
        TOKEN ident = CurTok;
        TOKEN next = getNextToken();
        if (next.type == ASSIGN) {
            //DebugPrint("found assign");
            getNextToken();
            auto expr = parse_expr();
            if (expr == nullptr) {
                return nullptr;
            }
            return std::make_unique<AssignExpr>(ident, std::move(expr));
        } else {
            //DebugPrint("no assign");
            putBackToken(next);
            CurTok = ident;
        }
    }

    //DebugPrint("just doing expr");
    auto LHS = parse_primary();
    if (LHS == nullptr) {
        return nullptr;
    }

    return parse_bin_op_rhs(0, std::move(LHS));
}

// Parse Unary / Literal 
std::unique_ptr<ASTNode> parse_primary() {
    switch (CurTok.type) {
        case MINUS:
        case NOT: {
            //DebugPrint("unary");
            TOKEN op = CurTok;
            getNextToken();
            auto literal = parse_primary();
            if (literal == nullptr) {
                return nullptr;
            }
            return std::make_unique<UnaryOp>(op, std::move(literal));
        }
        case BOOL_LIT:
        case FLOAT_LIT:
        case INT_LIT:
        case IDENT:
        case LPAR:
            //DebugPrint("literal");
            return parse_literal();

        default:
            return ThrowError<ASTNode>("Expected unary operation\"-\", \"!\"; or literal");
    }
}

std::unique_ptr<ASTNode> parse_literal() {
    std::unique_ptr<ASTNode> literal;
    switch (CurTok.type) {
        case BOOL_LIT:
            // Literal bool value
            literal = std::make_unique<BoolLit>(CurTok, BoolVal);
            getNextToken();
            break;
        case FLOAT_LIT:
            // Floating point value
            literal = std::make_unique<FloatLit>(CurTok, FloatVal);
            getNextToken();
            break;
        case INT_LIT:
            // Integer literal value
            literal = std::make_unique<IntLit>(CurTok, IntVal);
            getNextToken();
            break;
        case LPAR:
            // Bracketed Expression
            getNextToken();
            literal = parse_expr();
            if (literal == nullptr) {
                return nullptr;
            }
            if (!EatToken(RPAR)) {
                return ThrowError<ASTNode>("Expected \")\" after nested expr");
            }
            break;
        case IDENT: {
            // Variable or Function Call
            TOKEN ident = CurTok;
            // DebugPrint("Found ident!");
            getNextToken();
            // DebugPrint("Found Ident, next tok is");
            switch (CurTok.type) {
                case LPAR: {
                    // Must be a function call
                    getNextToken();
                    std::vector<std::unique_ptr<ASTNode>> arg_list;
                    if (parse_args(arg_list) < 0) {
                        return nullptr;
                    }
                    if (!EatToken(RPAR)) {
                        return ThrowError<ASTNode>("Expected \")\" after argument list");
                    }
                    return std::make_unique<FunctionCallLiteral>(ident, std::move(arg_list));
                }
                case COMMA:
                case RPAR:
                case MINUS:
                case MOD:
                case DIV:
                case ASTERIX:
                case PLUS:
                case GT:
                case GE:
                case LT:
                case LE:
                case NE:
                case EQ:
                case AND:
                case OR:
                case SC:
                    // Just a variable
                    return std::make_unique<VariableLit>(ident);
                default:
                    return ThrowError<ASTNode>("Expected \"(\" followed by arg list or operator and epxression");
            }
        }
        default:
            return ThrowError<ASTNode>("Expected literal value, function call or nested expr");
    }

    return literal;
}

// Parse the OP RHS part of the expressions
std::unique_ptr<ASTNode> parse_bin_op_rhs(int exprPrec, std::unique_ptr<ASTNode> LHS) {
    while (1) {
        //DebugPrint("loop start: expr so far=" + std::to_string(exprPrec));
        
        int prec = getTokPrecedence();
        //DebugPrint("current prec" + std::to_string(prec));
        if (prec < exprPrec) {
            return LHS;
        }
        
        TOKEN binop = CurTok;
        getNextToken();

        auto RHS = parse_primary();
        if (RHS == nullptr) {
            return nullptr;
        }
        int nextPrec = getTokPrecedence();
        //DebugPrint("next prec" + std::to_string(nextPrec));
        if (prec < nextPrec) {
            //DebugPrint("recurse and parse again");
            RHS = parse_bin_op_rhs(prec + 1, std::move(RHS));
            if (RHS == nullptr) {
                return nullptr;
            }
        }

        LHS = std::make_unique<BinaryOp>(std::move(LHS), binop, std::move(RHS));
    }
}

int parse_args(std::vector<std::unique_ptr<ASTNode>> &list) {
    switch (CurTok.type) {
        case RPAR: 
            return 0;
        case COMMA:
            if (list.size() > 0) {
                getNextToken();
                auto expr = parse_expr();
                if (expr == nullptr) {
                    return -1;
                }
                list.push_back(std::move(expr));
                return parse_args(list);
            }
            ThrowErrorV("Expected expression for argument");
            return -1;
        case BOOL_LIT:
        case FLOAT_LIT:
        case INT_LIT:
        case IDENT:
        case LPAR:
        case MINUS:
        case NOT: 
            if (list.size() == 0) {
                auto expr = parse_expr();
                if (expr == nullptr) {
                    return -1;
                }
                list.push_back(std::move(expr));
                return parse_args(list);
            }
            ThrowErrorV("Expected \",\" following other arguments");
            return -1;
        default:
            ThrowErrorV("Expected expression, comma continuing arg list or end of arg list");
            return -1;
    }
}