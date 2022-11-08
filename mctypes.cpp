#include "mccomp.hh"

using namespace llvm;
using namespace llvm::sys;

// Integer type is a 32-bit signed integer type
Type* getIntegerType() {
    return Type::getInt32Ty(TheContext);
}

bool isIntegerType(Value* val) {
    return val->getType()->isIntegerTy(32);
} 

// Boolean type is an unsigned 1-bit integer type
Type* getBooleanType() {
    return Type::getInt1Ty(TheContext);
}

bool isBooleanType(Value* val) {
    return val->getType()->isIntegerTy(1);
}

// Floating point is the IEEE single float
Type* getFloatingType() {
    return Type::getFloatTy(TheContext);
}

bool isFloatingType(Value* val) {
    return val->getType()->isFloatTy();
}

// Void type is void?
Type* getVoidType() {
    return Type::getVoidTy(TheContext);
}

bool isVoidType(Value* val) {
    return val->getType()->isVoidTy();
}

bool isPointerType(Value* val) {
    return val->getType()->isPointerTy();
}

bool isOpaquePointerType(Value* val) {
    return val->getType()->isOpaquePointerTy();
}

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
            fprintf(stderr, "Invalid type \"%s\" expected bool, float, int void", tok.lexeme.c_str());
            return nullptr;
    }
}

Value* convertToBool(Value* value) {
    if (isBooleanType(value)) {
        return value;
    }

    if (isFloatingType(value)) {
        return Builder.CreateFCmpUNE(value, ConstantFP::get(TheContext, APFloat(0.0)), "float_bool_conv");
    }

    if (isIntegerType(value)) {
        return Builder.CreateICmpNE(value, ConstantInt::get(TheContext, APInt(32, 0, true)), "int_bool_conv");
    }

    fprintf(stderr, "Couldn't coerce type \"%d\" to boolean", value->getType()->getTypeID());
    return nullptr;
    // if (value->getType()->isFloatTy()) {
    //     // If value != 0 then it's truthy else false
    //     printf("Implicit coercion of floating point value to boolean");
    //     return Builder.CreateFCmpUNE(value, ConstantFP::get(TheContext, APFloat(0.0)), "float_bool_conv");
    // } else if (value->getType()->isIntegerTy()) {
    //     if (value->getType()->getIntegerBitWidth() == 1) {
    //         return value;
    //     } else {
    //         // If val != 0 then value is true (1) else false (0)
    //         printf("Implicit coercion of integer value to boolean");
    //         return Builder.CreateICmpNE(value, ConstantInt::get(TheContext, APInt(value->getType()->getIntegerBitWidth(), 0, false)), "int_bool_conversion");
    //     }
    // } else {
    //     return ThrowValueError("Couldn't coerce type of value \"%s\" into \"bool\"", value->getType());
    // }
}

Value* integerToFloat(Value* ival) {
    return Builder.CreateSIToFP(ival, getFloatingType(), "int_float_conv");
}

// TODO: using cmp here might not actually return a 1-bit int value!!
Value* integerToBool(Value* ival) {
    return Builder.CreateICmpNE(ival, ConstantInt::get(TheContext, APInt(32, 0, true)), "int_bool_conv");
}

Value* floatToInteger(Value* fval) {
    return Builder.CreateFPToSI(fval, getIntegerType(), "float_int_conv");
}

// TODO: using cmp here might not actually return a 1-bit int value!!
Value* floatToBool(Value* fval) {
    return Builder.CreateFCmpONE(fval, ConstantFP::get(TheContext, APFloat(0.0)), "float_bool_conv");
}

Value* boolToInteger(Value* bval) {
    return Builder.CreateBitCast(bval, getIntegerType(), "bool_int_conv");
}

Value* boolToFloat(Value* bval) {
    return Builder.CreateUIToFP(bval, getFloatingType(), "bool_float_conv");
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
    // TODO: Change from SIToFP depending on int type signedness

    // if (lhs->getType()->isFloatTy()) {
    //     // Convert RHS to float
    //     return std::pair(lhs, Builder.CreateSIToFP(rhs, lhs->getType(), "convert_to_float"));
    // }

    // if (rhs->getType()->isFloatTy()) {
    //     // Convert LHS to float
    //     return std::pair(Builder.CreateSIToFP(lhs, rhs->getType(), "convert_to_float"), rhs);
    // }

    // if (lhs->getType() == rhs->getType()) {
    //     return std::pair(lhs, rhs);
    // }

    // if (lhs->getType()->canLosslesslyBitCastTo(rhs->getType())) {
    //     return std::pair(Builder.CreateBitCast(lhs, rhs->getType(), "cast_a_b"), rhs);
    // }

    // if (rhs->getType()->canLosslesslyBitCastTo(lhs->getType())) {
    //     return std::pair(lhs, Builder.CreateBitCast(rhs, lhs->getType(), "case_b_a"));
    // }

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
    fprintf(stderr, "%d", isPointerType(lhs));
    fprintf(stderr, "%d", isOpaquePointerType(lhs));
    
    fprintf(stderr, "Right Type: id: %d\n", lhs->getType()->getTypeID());
    return std::pair(nullptr, nullptr);

    // // Both are not floats (i.e. ints)
    // if (lhs->getType()->getIntegerBitWidth() > rhs->getType()->getIntegerBitWidth()) {
    //     // Convert RHS to LHS bit width
    //     // Builder.CreateIntCast(rhs, lhs->getType(), lhs->getType()->canLosslesslyBitCastTo());
    // } else if (lhs->getType()->getIntegerBitWidth() < rhs->getType()->getIntegerBitWidth()) {
    //     // Convert LHS to RHS bit width
    // }

    // return std::pair(lhs, rhs);
}