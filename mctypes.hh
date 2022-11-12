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

Value* convertToBool(Value* value, int lineNo = -1, int columnNo = -1);
Value* convertToInteger(Value* value, int lineNo = -1, int columnNo = -1);
Value* convertToFloat(Value* value, int lineNo = -1, int columnNo = -1);

Constant* getDefaultValue(Type* typ);

Value* integerToFloat(Value* ival);
Value* integerToBool(Value* ival, int lineNo = -1, int columnNo = -1);
Value* floatToInteger(Value* fval, int lineNo = -1, int columnNo = -1);
Value* floatToBool(Value* fval, int lineNo = -1, int columnNo = -1);
Value* boolToInteger(Value* bval);
Value* boolToFloat(Value* bval);

std::pair<Value*, Value* > convertToWider(Value* lhs, Value* rhs);