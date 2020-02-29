#pragma once
#include <stdint.h>
#include "string.h"
#include "config.hpp"

bool isTypeTruthy (IType type);
bool isTypeInt (IType type);
void skipArg (uint8_t** f);

#ifndef min
  #define min(a,b) (a<b?a:b)
#endif

uint8_t _log10 (uint32_t);
int32_t _pow   (int32_t n, uint8_t p);

//Provide at least 11 bytes; returns length
uint8_t int2chars (uint8_t*, int32_t);

int32_t  readNum   (uint8_t*, uint8_t);
uint32_t readUNum  (uint8_t*, uint8_t);
void     writeNum  (uint8_t*, int32_t, uint8_t);
void     writeUNum (uint8_t*, uint32_t, uint8_t);
