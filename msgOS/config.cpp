#include "config.hpp"

funclen constByteLen (IType t, uint8_t* body) {
  switch (t) {
    case Val_True: case Val_False: return 0;
    case Val_Str:  return body ? strlen((const char*)body) + 1 : 0;
    case Val_Byte: return 1;
    case Val_Word: return 2;
    case Val_Int:  return 4;
    case Val_Long: case Val_Deci: return 8;
    case Val_Nil: return 0;
  }
  return 0;
}
