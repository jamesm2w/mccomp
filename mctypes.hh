#include "mccomp.hh"

using namespace llvm;
using namespace llvm::sys;

Type* getIntegerType();
bool isIntegerType(Value* val);

Type* getBooleanType();
bool isBooleanType(Value* val);

Type* getFloatingType();
bool isFloatingType(Value* val);

Type* getVoidType();
bool isVoidType(Value* val);

Type* getTokType(TOKEN tok);

Value* convertToBool(Value* value);

std::pair<Value*, Value* > convertToWider(Value* lhs, Value* rhs);