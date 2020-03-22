#pragma once
#include <stdint.h>
#include "string.h"

#define USE_SERIAL true

//MEM_SIZE: Arduino SRAM size
//#define MEM_SIZE 262144 //256k
//#define MEM_SIZE 131072 //128k
//#define MEM_SIZE 65536  //64k
  #define MEM_SIZE 32768  //32k; MKRZero, Feather M0
//#define MEM_SIZE 16384  //16k
//#define MEM_SIZE 8192   //8k;  Mega 2560
//#define MEM_SIZE 2048   //2k
//#define MEM_SIZE 512    //512

//CHIKA_SIZE: all Chika programs' memory
#define CHIKA_SIZE (uint32_t)(MEM_SIZE * .5)

#define MIN_NUM_PROG     4
#define MAX_NUM_PROG     8
#define MAX_PROG_RAM CHIKA_SIZE / MIN_NUM_PROG


#ifdef SDCARD_SS_PIN
  #define SD_CARD_PIN SDCARD_SS_PIN
#endif
#ifndef SDCARD_SS_PIN
  #define SD_CARD_PIN 4
#endif

typedef uint16_t proglen;
typedef uint16_t itemlen;
typedef uint8_t  prognum;
typedef uint16_t itemnum;
typedef uint32_t bytenum;
typedef uint16_t funcnum;
typedef uint16_t funclen;
typedef uint8_t  argnum;
typedef uint16_t bindnum;
typedef uint16_t strilen;
typedef uint16_t vectlen;


//NOTE: when adding a new value type or op longer than 1B
//  ensure its implementation in constByteLen
enum IType : uint8_t {
  Form_Eval = 0x00,
  Form_If = 0x01, Form_Or = 0x02, Form_And = 0x03, Form_Case = 0x04,
  FORMS_END = 0x04,

  Val_True  = 0x05, Val_False = 0x06,
  Val_Str   = 0x07,
  Param_Val = 0x08,
  Bind_Mark = 0x09, //e.g. test=
  Bind_Val  = 0x0A, //     test
  Val_Vec   = 0x0B,
  Val_Blob  = 0x0C,
  Val_U08   = 0x10,
  Val_U16   = 0x11,
  Val_I32   = 0x12,
  Val_Char  = 0x13,
  Val_Args  = 0x19,
  Var_Op    = 0x1A,
  Var_Func  = 0x1B,
  Val_Nil   = 0x1E,

  OPS_START = 0x1F,
  Op_Func   = 0x22,
  Op_If     = 0x23,
  Op_Or     = 0x24,
  Op_And    = 0x25,
  Op_Case   = 0x26,
  Op_Not    = 0x27,
  Op_Bind   = 0x2A,
  Op_Param  = 0x2B,
  Op_Return = 0x2E,
  Op_Recur  = 0x2F,
  Op_Equal  = 0x30, Op_Equit  = 0x31,
  Op_Nequal = 0x32, Op_Nequit = 0x33,
  Op_LT     = 0x34, Op_LTE    = 0x35,
  Op_GT     = 0x36, Op_GTE    = 0x37,
  Op_Add    = 0x38, Op_Sub    = 0x39,
  Op_Mult   = 0x3A, Op_Div    = 0x3B,
  Op_Mod    = 0x3C, Op_Pow    = 0x3D,
  Op_BNot   = 0x40, Op_BAnd   = 0x41,
  Op_BOr    = 0x42, Op_BXor   = 0x43,
  Op_LShift = 0x44, Op_RShift = 0x45,
  Op_PinMod = 0x60,
  Op_DigIn  = 0x61, Op_DigOut = 0x62,
  Op_AnaIn  = 0x63, Op_AnaOut = 0x64,
  Op_Read   = 0x6A, Op_Write  = 0x6B,
  Op_Append = 0x6C, Op_Delete = 0x6D,
  Op_Str    = 0xA0,
  Op_Type   = 0xAA,
  Op_Cast   = 0xAB,
  Op_Vec    = 0xB0,
  Op_Nth    = 0xB1,
  Op_Len    = 0xB2,
  Op_Sect   = 0xB3,
  Op_BSect  = 0xB4,
  Op_Burst  = 0xBA,
  Op_Reduce = 0xBB,
  Op_Map    = 0xBC,
  Op_For    = 0xBD,
  Op_Loop   = 0xBE,
  Op_Val    = 0xCD, Op_Do     = 0xCE,
  Op_MsNow  = 0xE0,
  Op_Sleep  = 0xE1,
  Op_Print  = 0xEE,
  Op_Debug  = 0xEF,
  Op_Load   = 0xF0,
  Op_Halt   = 0xFF
};

funclen constByteLen (IType, uint8_t* = nullptr);
IType fitInt (uint8_t);
