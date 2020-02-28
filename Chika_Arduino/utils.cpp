#include "utils.hpp"

bool isTypeTruthy (IType type) {
  return type != Val_Nil && type != Val_False;
}

bool isTypeInt (IType type) {
  return type >= Val_U08 && type <= Val_I32;
}

void skipArg (uint8_t** f) {
  //Is a value?
  if (**f > FORMS_END) {
    IType type = (IType)**f;
    *f += constByteLen(type, ++*f);
    return;
  }
  //It's a form - skip f to the end of it
  uint8_t nForm = 0;
  do {
    //If it's a form, increment form counter
    if (**f <= FORMS_END) {
      ++*f;
      ++nForm;
    } else {
      //If an op, decrement form counter
      if (**f >= OPS_START) --nForm;
      IType type = (IType)**f;
      *f += constByteLen(type, ++*f);
    }
  } while (nForm);
}

uint8_t _log10 (uint32_t v) {
  return (v >= 1000000000) ? 9 : (v >= 100000000) ? 8 : 
         (v >= 10000000) ? 7   : (v >= 1000000) ? 6 : 
         (v >= 100000) ? 5     : (v >= 10000) ? 4 :
         (v >= 1000) ? 3       : (v >= 100) ? 2 :
         (v >= 10) ? 1         : 0; 
}

int32_t readNum (uint8_t* b, uint8_t len) {
  return *(int32_t*)b & ((uint32_t)-1 >> ((4 - len) * 8));
}

//Provide at least 11 bytes; returns length
uint8_t int2chars (uint8_t* str, int32_t n) {
  if (n == 0) {
    str[0] = '0';
    return 1;
  }
  uint8_t offset = 0;
  if (n < 0) {
    str[0] = '-';
    n = -n;
    offset = 1;
  }
  uint8_t numDigits = _log10(n) + 1;
  uint8_t c = numDigits + offset;
  do {
    str[c - 1] = (n % 10) + 48;
    n /= 10;
  } while(--c > offset);
  return numDigits + offset;
}
