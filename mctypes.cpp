#include "mccomp.hh"

using namespace llvm;
using namespace llvm::sys;

// Integer type is a 32-bit signed integer type
Type* getIntegerType() {
    return Type::getInt32Ty(TheContext);
}

// Is the type integer
bool isIntegerType(Value* val) {
    return val->getType()->isIntegerTy(32);
} 

// Boolean type is an unsigned 1-bit integer type
Type* getBooleanType() {
    return Type::getInt1Ty(TheContext);
}

// Is the type a boolean
bool isBooleanType(Value* val) {
    return val->getType()->isIntegerTy(1);
}

// Floating point is the IEEE single float
Type* getFloatingType() {
    return Type::getFloatTy(TheContext);
}

// Is the type single floating point (float)
bool isFloatingType(Value* val) {
    return val->getType()->isFloatTy();
}

// Void type is void?
Type* getVoidType() {
    return Type::getVoidTy(TheContext);
}

// Is the type void
bool isVoidType(Value* val) {
    return val->getType()->isVoidTy();
}

// Get the type for the token type
Type* getTokType(TOKEN tok) {
    switch (tok.type) {
        case BOOL_TOK:
            return getBooleanType();
        case FLOAT_TOK:
            return getFloatingType();
        case INT_TOK:
            return getIntegerType();
        case VOID_TOK:
            return getVoidType();
        default:
            fprintf(stderr, "Codegen Error On Line %d Column %d: Invalid type \"%s\" expected bool, float, int void", tok.lineNo, tok.columnNo, tok.lexeme.c_str());
            return nullptr;
    }
}

// Convert the type into a boolean value
Value* convertToBool(Value* value, int lineNo, int columnNo) {
    if (!value) return nullptr;

    if (isBooleanType(value)) {
        return value;
    }

    if (isFloatingType(value)) {
        return floatToBool(value);
        // fprintf(stderr, "Codegen Warning On Line %d Column %d: Implicit conversion from float to boolean", lineNo, columnNo);
        // return Builder.CreateFCmpUNE(value, ConstantFP::get(TheContext, APFloat(0.0)), "float_bool_conv");
    }

    if (isIntegerType(value)) {
        return integerToBool(value);
        // fprintf(stderr, "Codegen Warning On Line %d Column %d: Implicit conversion from integer to boolean", lineNo, columnNo);
        // return Builder.CreateICmpNE(value, ConstantInt::get(TheContext, APInt(32, 0, true)), "int_bool_conv");
    }

    fprintf(stderr, "Codegen Error On Line %d Column %d: Couldn't coerce type \"%d\" to boolean", lineNo, columnNo, value->getType()->getTypeID());
    return nullptr;
}

// Convert an integer to float (Widening conversion)
Value* integerToFloat(Value* ival) {
    return Builder.CreateSIToFP(ival, getFloatingType(), "int_float_conv");
}

// Convert an integer to a boolean (Narrowing Conversion)
Value* integerToBool(Value* ival, int lineNo, int columnNo) {
    fprintf(stderr, "Codegen Warning On Line %d Column %d: Implicit conversion from integer to boolean", lineNo, columnNo);    
    return Builder.CreateICmpNE(ival, ConstantInt::get(TheContext, APInt(32, 0, true)), "int_bool_conv");
}

// Convert a float down to an integer (Narrowing Conversion)
Value* floatToInteger(Value* fval, int lineNo, int columnNo) {
    fprintf(stderr, "Codegen Error On Line %d Column %d: Implicit conversion from float to integer", lineNo, columnNo);
    return Builder.CreateFPToSI(fval, getIntegerType(), "float_int_conv");
}

// Convert a float to a boolean value (Narrowing Conversion)
Value* floatToBool(Value* fval, int lineNo, int columnNo) {
    fprintf(stderr, "Codegen Warning On Line %d Column %d: Implicit conversion from float to boolean", lineNo, columnNo);
    return Builder.CreateFCmpONE(fval, ConstantFP::get(TheContext, APFloat(0.0)), "float_bool_conv");
}

// Convert a boolean to integer value (Widening Conversion)
Value* boolToInteger(Value* bval) {
    return Builder.CreateBitCast(bval, getIntegerType(), "bool_int_conv");
}

// Convert a boolean to a floating value (Widening Conversion)
Value* boolToFloat(Value* bval) {
    return Builder.CreateUIToFP(bval, getFloatingType(), "bool_float_conv");
}

// Get Default Value for a type 
Constant* getDefaultValue(Type* typ) {
    if (typ->isIntegerTy(1)) {
        return ConstantInt::get(getBooleanType(), APInt(1, 0, false));
    } else if (typ->isIntegerTy(32)) {
        return ConstantInt::get(getIntegerType(), APInt(32, 0, false));
    } else if (typ->isFloatTy()) {
        return ConstantFP::get(TheContext, APFloat((float)0.0));
    } else {
        return nullptr;
    }
}

// General Conversion Table: if lhs, rhs then convert both to : 
//lhs\rhs bool   int   float
//   bool bool   int   float
//    int int    int   float
//  float float float  float

// Conversions needed:
// bool -> int 
// bool -> float
// int  -> float

// Special Operators: 
// NOT "!"  -- Convert X->bool (0->1; 1->0)
// AND "&&" -- Convert X, Y -> bool
// OR  "||" -- Convert X, Y -> bool

// performs necessary conversions s.t. lhs, and rhs are the same types (the wider of the two)
std::pair<Value*, Value* > convertToWider(Value* lhs, Value* rhs) {
    if (!lhs || !rhs) return std::pair(nullptr, nullptr);
    
    if (isFloatingType(lhs)) {
        if (isFloatingType(rhs)) {
            return std::pair(lhs, rhs);
        } else if (isIntegerType(rhs)) {
            return std::pair(lhs, integerToFloat(rhs));
        } else if (isBooleanType(rhs)) {
            return std::pair(lhs, boolToFloat(rhs));
        }
    } else if (isIntegerType(lhs)) {
        if (isIntegerType(rhs)) {
            return std::pair(lhs, rhs);
        } else if (isBooleanType(rhs)) {
            return std::pair(lhs, boolToInteger(rhs));
        }
        // RHS = Float => fall through to bottom case
    }

    if (isFloatingType(rhs)) {
        if (isFloatingType(lhs)) {
            return std::pair(lhs, rhs);
        } else if (isIntegerType(lhs)) {
            return std::pair(integerToFloat(lhs), rhs);
        } else if (isBooleanType(lhs)) {
            return std::pair(boolToFloat(lhs), rhs);
        }
    } else if (isIntegerType(rhs)) {
        if (isIntegerType(lhs)) {
            return std::pair(lhs, rhs);
        } if (isBooleanType(lhs)) {
            return std::pair(boolToInteger(lhs), rhs);
        }
        // RHS: int /\ LHS: float => handled above
    }

    if (isBooleanType(lhs) && isBooleanType(rhs)) {
        return std::pair(lhs, rhs);
    }

    // surely if one isn't float, and there's not a (bool)->signed int bit cast between then
    // what types would we be actually dealing with??
    fprintf(stderr, "Not sure how to coerce these two types, can someone help??\t");
    fprintf(stderr, "Left Type: id: %d\t", lhs->getType()->getTypeID());
    fprintf(stderr, "%d", isIntegerType(lhs));
    fprintf(stderr, "%d", isFloatingType(lhs));
    fprintf(stderr, "%d", isBooleanType(lhs));
    fprintf(stderr, "%d", isVoidType(lhs));
    
    fprintf(stderr, "Right Type: id: %d\n", lhs->getType()->getTypeID());
    return std::pair(nullptr, nullptr);
}