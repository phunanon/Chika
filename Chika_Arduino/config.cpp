#include "config.hpp"

//NOTE: Val_Vec only exist as an item, and therefore is not included.
funclen constByteLen (IType t, uint8_t* body) {
  switch (t) {
    case Val_True: case Val_False: return 0;
    case Val_Str:  return body ? strlen((const char*)body) + 1 : 0;
    case Param_Val: return sizeof(argnum);
    case Bind_Mark: case Bind_Val: return sizeof(bindnum);
    case Val_U08: return 1;
    case Val_U16: return 2;
    case Val_I32: return 4;
    case Val_Char: return 1;
    case Var_Op: return 1;
    case Var_Func: return sizeof(funcnum);
    case Val_Nil: return 0;
    case Op_Func: return sizeof(funcnum);
    case Op_Bind: return sizeof(bindnum);
    case Op_Param: return sizeof(argnum);
  }
  return 0;
}

IType fitInt (uint8_t nBytes) {
  switch (nBytes) {
    case 1: return Val_U08;
    case 2: return Val_U16;
    case 3: case 4: return Val_I32;
  }
  return Val_Nil;
}