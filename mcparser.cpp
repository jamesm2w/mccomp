#include "mccomp.hh"

#include <memory>
#include <string>
#include <stdexcept>

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

  return CurTok = temp;
}

void putBackToken(TOKEN tok) { tok_buffer.push_front(tok); }

//===----------------------------------------------------------------------===//
// Recursive Descent Parser - Function call for each production
//===----------------------------------------------------------------------===//

std::unique_ptr<ASTNode> LogError(const char *Str) {
    fprintf(stderr, "Error: %s\n", Str);
    return nullptr;
}

std::unique_ptr<ASTNode> LogErrorS(const std::string Str) {
    fprintf(stderr, "Error: %s\n", Str.c_str());
    return nullptr;
}

int errorCode = 0;

// Throw an error from a void function
template<typename ... Args>
void ThrowErrorV( const std::string& format, Args ... args ) {

    errorCode = 1;

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
template<typename ... Args>
std::unique_ptr<ASTNode> ThrowError( const std::string &format, Args ... args) {
    ThrowErrorV(format, args ...);
    return nullptr;
}

// Print out the current token.
void DebugPrint() {
    fprintf(stderr, "Debug: CurTok[Type %d Line %d Col %d Lexeme \"%s\"]\n", CurTok.type, CurTok.lineNo, CurTok.columnNo, CurTok.lexeme.c_str());
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
std::unique_ptr<ASTNode> parser() {
    std::vector<std::unique_ptr<ASTNode>> extern_list;
    std::vector<std::unique_ptr<ASTNode>> decl_list;
    DebugPrint();

    if (parse_extern_list(extern_list) < 0) {
        return nullptr;
    };
    
    DebugPrint();
    
    if (parse_decl_list(decl_list) < 0) {
        return nullptr;
    }

    return std::make_unique<Program>(std::move(extern_list), std::move(decl_list));
}

// ExternList  ::= Extern ExternList'
// ExternList' ::= Extern ExternList'
//               | epsilon
int parse_extern_list(std::vector<std::unique_ptr<ASTNode>> &list) {
    switch (CurTok.type) {
        case EXTERN: {
            std::unique_ptr<ASTNode> extern_item = parse_extern();
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
            return 0;
        default:
            ThrowErrorV("Expected one of \"extern\", \"void\", \"bool\", \"float\", \"int\"");
            return -1;
    }
}

// Extern ::= extern TypeSpec IDENT ( Params ) ;
std::unique_ptr<ASTNode> parse_extern() {
    
    if (!EatToken(EXTERN)) {
        return ThrowError("Expected \"extern\"");
    }

    TOKEN type = CurTok;
    getNextToken();
    
    TOKEN ident = CurTok;
    getNextToken();
    
    if (!EatToken(LPAR)) {
        return ThrowError("Expected \"(\"");
    }
    
    std::optional<std::vector<std::unique_ptr<ASTNode>>> params = parse_params();
    if (!params.has_value()) {
        return nullptr;
    }
    
    if (!EatToken(RPAR)) {
        return ThrowError("Expected \")\"");
    }
    
    if (!EatToken(SC)) {
        return ThrowError("Expected \";\"");
    }
    
    return std::make_unique<ExternDecl>(type, ident, std::move(params.value()));
}

// DeclList  ::= Decl DeclList'
// DeclList' ::= Decl DeclList'
//             | epsilon
int parse_decl_list(std::vector<std::unique_ptr<ASTNode>> &list) {
    switch (CurTok.type) {
        case VOID_TOK:
        case BOOL_TOK:
        case FLOAT_TOK:
        case INT_TOK: {
            std::unique_ptr<ASTNode> decl_item = parse_decl();
            if (decl_item == nullptr) {
                return -1;
            }
            list.push_back(std::move(decl_item));
            return parse_decl_list(list);
        }
        case EOF_TOK:
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
std::unique_ptr<ASTNode> parse_decl() {

    TOKEN type = CurTok;
    getNextToken();
    TOKEN ident = CurTok;
    getNextToken();

    if (type.type == VOID_TOK || CurTok.type == LPAR) {
        if (!EatToken(LPAR)) {
            return ThrowError("Expected \"(\"");
        }
        // Function Decl
        std::optional<std::vector<std::unique_ptr<ASTNode>>> params = parse_params();
        if (!params.has_value()) {
            return nullptr;
        }
        
        if (!EatToken(RPAR)) {
            return ThrowError("Expected \")\"");
        }

        auto block = parse_block();
        if (block == nullptr) {
            return nullptr;
        }

        return std::make_unique<GlobalDecl>(type, ident, std::move(params.value()), std::move(block));
    } else if (CurTok.type == SC) {

        if (!EatToken(SC)) {
            return ThrowError("Expected \";\"");
        }

        return std::make_unique<GlobalDecl>(type, ident);
    } else {
        return ThrowError("Expected \"(\" or \";\""); 
    }
}

//TODO: Error checking for params & rest of parser & ast printing

// Params ::= Paramlist
//          | void
// 		 | epsilon
std::optional<std::vector<std::unique_ptr<ASTNode>>> parse_params() {
    std::vector<std::unique_ptr<ASTNode>> list;

    switch (CurTok.type) {
        case VOID_TOK:
            getNextToken();
        case RPAR:
            return std::optional{ std::move(list) };
        case BOOL_TOK:
        case FLOAT_TOK:
        case INT_TOK:
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
int parse_param_list(std::vector<std::unique_ptr<ASTNode>> &list) {
    switch (CurTok.type) {
        case BOOL_TOK:
        case FLOAT_TOK:
        case INT_TOK:
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
            return 0;
        default:
            return -1;
    }
}

// Param ::= VarType IDENT
std::unique_ptr<ASTNode> parse_param() {
    TOKEN type = CurTok;
    getNextToken();
    TOKEN ident = CurTok;
    getNextToken();
    return std::make_unique<Param>(type, ident);
}

std::unique_ptr<ASTNode> parse_block() {
    std::vector<std::unique_ptr<ASTNode>> local_decls;
    std::vector<std::unique_ptr<ASTNode>> stmt_list;

    while (getNextToken().type != EOF_TOK) {
        fprintf(stderr, "DBG: Skipping\t");
        DebugPrint();
    }
    return std::make_unique<Block>(std::move(local_decls), std::move(stmt_list));
}