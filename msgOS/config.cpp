#include "config.hpp"

//NOTE: Val_Vec and Val_Dict only exist as items, and therefore are not included.
funclen constByteLen (IType t, uint8_t* body) {
  switch (t) {
    case Val_True: case Val_False: return 0;
    case Val_Str:  return body ? strlen((const char*)body) + 1 : 0;
    case Param_Val: return sizeof(argnum);
    case Bind_Var: case Var_Val: return sizeof(varnum);
    case Val_U08: return 1;
    case Val_U16: return 2;
    case Val_I32: return 4;
    case Var_Op: return 1;
    case Var_Func: return sizeof(funcnum);
    case Val_Nil: return 0;
    case Op_Func: return sizeof(funcnum);
    case Op_Var: return sizeof(varnum);
    case Op_Param: return sizeof(argnum);
  }
  return 0;
}
