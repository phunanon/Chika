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

int32_t _pow (int32_t n, uint8_t p) {
  int32_t sum = 1;
  for (uint8_t i = 0; i < p; ++i)
    sum = sum * n;
  return sum;
}

bool isDigit (char ch) {
  return ch >= '0' && ch <= '9';
}
bool isHexDigit (char ch) {
  return isDigit(ch) || (ch >= 'A' && ch <= 'F');
}

//All required because the Arduino doesn't like `*(type*)something`
int32_t readNum (uint8_t* b, uint8_t len) {
  if (!len) return 0;
  int32_t n = 0;
  memcpy(&n, b, sizeof(int32_t));
  return n & ((uint32_t)-1 >> ((4 - len) * 8));
}
uint32_t readUNum  (uint8_t* b, uint8_t len) {
  if (!len) return 0;
  uint32_t n = 0;
  memcpy(&n, b, len);
  return n;
}
void writeNum (uint8_t* b, int32_t i, uint8_t len) {
  memcpy(b, &i, len);
}
void writeUNum (uint8_t* b, uint32_t i, uint8_t len) {
  memcpy(b, &i, len);
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
  } while (--c > offset);
  return numDigits + offset;
}

int32_t chars2int (const char* str, bool isHex) {
  int32_t num = 0;
  bool isNeg = *str == '-';
  if (isNeg) ++str;
  while (isHex ? isHexDigit(*str) : isDigit(*str)) {
    num *= isHex ? 16 : 10;
    num += *str < 'A' ? (*str - '0') : (*str - 'A' + 10);
    ++str;
  }
  return num * (isNeg ? -1 : 1);
}