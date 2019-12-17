#pragma once
#include <stdint.h>
#include "string.h"

#define min(a,b) (a<b?a:b)

uint8_t _log10 (uint32_t);

//Provide at least 11 bytes; returns length
uint8_t int2chars (uint8_t*, int32_t);

int32_t readNum  (uint8_t*, uint8_t);
void    writeNum (uint8_t*, int32_t, uint8_t);
