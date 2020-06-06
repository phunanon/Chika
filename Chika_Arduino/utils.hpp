#pragma once
#include <stdint.h>
#include "string.h"
#include "config.hpp"

bool isTypeTruthy (IType type);
bool isTypeInt (IType type);
uint8_t* skipArg (uint8_t* f);

#ifndef min
  #define min(a,b) (a<b?a:b)
#endif

uint8_t _log10 (uint32_t);
int32_t _pow   (int32_t n, uint8_t p);
uint32_t _rand ();
bool isDigit (char);

//Provide at least 11 bytes; returns length
uint8_t int2chars (uint8_t*, int32_t);
int32_t chars2int (const char*, bool = false);

int32_t  readNum   (uint8_t*, uint8_t);
uint32_t readUNum  (uint8_t*, uint8_t);
void     writeNum  (uint8_t*, int32_t, uint8_t);
void     writeUNum (uint8_t*, uint32_t, uint8_t);
