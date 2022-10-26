#include "mccomp.hh"

using namespace llvm;
using namespace llvm::sys;

// Define some global variables
FILE *pFile;
std::string IdentifierStr; // Filled in if IDENT
int IntVal = 0;			   // Filled in if INT_LIT
bool BoolVal = false;      // Filled in if BOOL_LIT
float FloatVal = 0.0;      // Filled in if FLOAT_LIT
int lineNo = 0;
int columnNo = 0;

//===----------------------------------------------------------------------===//
// Code Generation
//===----------------------------------------------------------------------===//

static LLVMContext TheContext;
static IRBuilder<> Builder(TheContext);
static std::unique_ptr<Module> TheModule;

//===----------------------------------------------------------------------===//
// AST Printer  
//===----------------------------------------------------------------------===//

inline llvm::raw_ostream &operator << (llvm::raw_ostream &os, const ASTNode &ast) {
	os << ast.to_string("", "");
	return os;
}

//===----------------------------------------------------------------------===//
// Main driver code.
//===----------------------------------------------------------------------===//

int main(int argc, char **argv) {
	// Command Line input manager
	if (argc == 2) {
		pFile = fopen(argv[1], "r");
		if (pFile == NULL)
			perror("Error opening file");
	} else {
		std::cout << "Usage: ./code InputFile\n";
		return 1;
	}

	// Initialize line number and column numbers to zero
	lineNo = 1;
	columnNo = 1;

	// ---- Uncomment for Lexer print-out ---- 
	// getNextToken();
	// while (CurTok.type != EOF_TOK) {
	//   fprintf(stderr, "Token: %s with type %d\n", CurTok.lexeme.c_str(),
	//           CurTok.type);
	//   getNextToken();
	// }
	// fprintf(stderr, "Lexer Finished\n");

	// Make the module, which holds all the code.
	TheModule = std::make_unique<Module>("mini-c", TheContext);

	// Run the parser now.
	getNextToken();
	std::unique_ptr<Program> RootASTNode = parser();
	fprintf(stderr, "Parsing Finished\n");

	if (RootASTNode == nullptr) {
		fprintf(stderr, "Error in Parser\n");
		return 1;
	}

	llvm::outs() << *RootASTNode.get() << "\n";

	//********************* Start printing final IR **************************
	// Print out all of the generated code into a file called output.ll
	auto Filename = "output.ll";
	std::error_code EC;
	raw_fd_ostream dest(Filename, EC, sys::fs::OF_None);

	if (EC) {
		errs() << "Could not open file: " << EC.message();
		return 1;
	}
	// TheModule->print(errs(), nullptr); // print IR to terminal
	TheModule->print(dest, nullptr);
	//********************* End printing final IR ****************************

	fclose(pFile); // close the file that contains the code that was parsed
	return 0;
}
