#include "mccomp.hh"

#include <memory>
#include <string>
#include <stdexcept>

// StackOverflow code for string formatting
template<typename ... Args>
std::string string_format( const std::string& format, Args ... args )
{
    int size_s = std::snprintf( nullptr, 0, format.c_str(), args ... ) + 1; // Extra space for '\0'
    // if( size_s <= 0 ){ throw std::runtime_error( "Error during formatting." ); }
    auto size = static_cast<size_t>( size_s );
    std::unique_ptr<char[]> buf( new char[ size ] );
    std::snprintf( buf.get(), size, format.c_str(), args ... );
    return std::string( buf.get(), buf.get() + size - 1 ); // We don't want the '\0' inside
}

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

void debug_print() {
    fprintf(stderr, "Debug: CurTok Type: %d Lexeme %s Line %d Col %d\n", CurTok.type, CurTok.lexeme.c_str(), CurTok.lineNo, CurTok.columnNo);
}

/* Add function calls for each production */

// Program ::= ExternList DeclList
//   		 | DeclList
std::unique_ptr<ASTNode> parser() {
    std::vector<std::unique_ptr<ASTNode>> extern_list;
    std::vector<std::unique_ptr<ASTNode>> decl_list;
    debug_print();
    parse_extern_list(extern_list);
    debug_print();
    parse_decl_list(decl_list);
    debug_print();
    return std::make_unique<Program>(std::move(extern_list), std::move(decl_list));
}

// ExternList  ::= Extern ExternList'
// ExternList' ::= Extern ExternList'
//               | epsilon
void parse_extern_list(std::vector<std::unique_ptr<ASTNode>> &list) {
    switch (CurTok.type) {
        case EXTERN:
            list.push_back(std::move(parse_extern()));
            debug_print();
            parse_extern_list(list);
            debug_print();
            break;
        case VOID_TOK:
        case BOOL_TOK:
        case FLOAT_TOK:
        case INT_TOK:
            return;
        default:
            debug_print();
            LogErrorS(string_format("Expected extern or following declaration. Got %s", CurTok.lexeme.c_str()));
            return;
    }
}

// Extern ::= extern TypeSpec IDENT ( Params ) ;
std::unique_ptr<ASTNode> parse_extern() {
    if (CurTok.type != EXTERN) {
        return LogError("Syntax Error: Expected extern keyword");
    }
    TOKEN type = getNextToken();
    debug_print();
    TOKEN ident = getNextToken();
    debug_print();
    if (getNextToken().type != LPAR) {
        return LogError("Syntax Error: Expected open bracket");
    }
    debug_print();
    getNextToken(); // EAT LPAR
    debug_print();
    std::vector<std::unique_ptr<ASTNode>> params = parse_params();
    if (CurTok.type != RPAR) {
        return LogError("Syntax Error: Expected close bracket");
    }
    debug_print();
    if (getNextToken().type != SC) {
        return LogError("Syntax Error: Expected semicolon to end extern");
    }
    debug_print();
    getNextToken(); // Eat semicolon
    debug_print();
    return std::make_unique<ExternDecl>(type, ident, std::move(params));
}

// DeclList  ::= Decl DeclList'
// DeclList' ::= Decl DeclList'
//             | epsilon
void parse_decl_list(std::vector<std::unique_ptr<ASTNode>> &list) {
    switch (CurTok.type) {
        case VOID_TOK:
        case BOOL_TOK:
        case FLOAT_TOK:
        case INT_TOK:
            list.push_back(std::move(parse_decl()));
            parse_decl_list(list);
            break;
        case EOF_TOK:
            if (list.size() >= 1) { // Need at least one decl 
                return;
            }
        default:
            LogErrorS(string_format("Syntax Error: Expected keyword or EOF. Got %s", CurTok.lexeme.c_str()));
            return;
    }
}

// Decl ::= VarType IDENT Decl'
//        | void IDENT FunDecl
// Decl' ::= ;
//         | FunDecl
// FunDecl ::= ( Params ) Block
std::unique_ptr<ASTNode> parse_decl() {

    TOKEN type = CurTok;
    TOKEN ident = getNextToken();
    getNextToken(); // Eat identifier

    if (type.type == VOID_TOK || getNextToken().type == LPAR) {
        // Function Decl
        std::vector<std::unique_ptr<ASTNode>> params = parse_params();
        if (getNextToken().type != RPAR) {
            return LogError("Syntax Error: Expected right bracket end of params");
        }
        auto block = parse_block();
        return std::make_unique<GlobalDecl>(type, ident, std::move(params), std::move(block));

    } else if (CurTok.type == SC) {
        getNextToken(); // CONSUME
        return std::make_unique<GlobalDecl>(type, ident);

    } else {
        return LogErrorS(string_format("Syntax Error: Expected ; or function declaration got %s", CurTok.lexeme.c_str())); 
    }
}

// Params ::= Paramlist
//          | void
// 		 | epsilon
std::vector<std::unique_ptr<ASTNode>> parse_params() {
    std::vector<std::unique_ptr<ASTNode>> list;
    switch (CurTok.type) {
        case VOID_TOK:
            getNextToken();
        case RPAR:
            return list;
        case BOOL_TOK:
        case FLOAT_TOK:
        case INT_TOK:
            parse_param_list(list);
            return list;
        default:
            LogErrorS(string_format("Syntax Error: Expected VOID, ')' or Parameter. Got %s", CurTok.lexeme.c_str()));
            return list;
    }
}

// ParamList  ::= Param ParamList'
// ParamList' ::= , Param ParamList'
// 			 | epsilon
void parse_param_list(std::vector<std::unique_ptr<ASTNode>> &list) {
    switch (CurTok.type) {
        case BOOL_TOK:
        case FLOAT_TOK:
        case INT_TOK:
            if (list.size() == 0) {
                list.push_back(std::move(parse_param()));
                parse_param_list(list);
            
            } else {
                LogError("Syntax Error: Expected a COMMA since param list has size >= 1");
                return;
            }
            break;
        case COMMA:
            if (list.size() >= 1) {
                getNextToken(); 
                list.push_back(std::move(parse_param()));
            } else {
                LogError("Syntax Error: Expected BOOL, FLOAT, INT since param list has size < 1");
                return;
            }
            break;
        case RPAR:
            return;
        default:
            LogErrorS(string_format("Syntax Error: Unexpected token %s. Expected parameter or continuation of list", CurTok.lexeme.c_str()));
            return;
    }
}

// Param ::= VarType IDENT
std::unique_ptr<ASTNode> parse_param() {
    TOKEN type = getNextToken();
    TOKEN ident = getNextToken();
    return std::make_unique<Param>(type, ident);
}

std::unique_ptr<ASTNode> parse_block() {
    std::vector<std::unique_ptr<ASTNode>> local_decls;
    std::vector<std::unique_ptr<ASTNode>> stmt_list;

    return std::make_unique<Block>(std::move(local_decls), std::move(stmt_list));
}