#pragma once
#include <stdint.h>
#include "string.h"

#define USE_SERIAL true
#define USE_DEBUGGING true

//MEM_SIZE: Arduino SRAM size
//#define MEM_SIZE 262144 //256k
//#define MEM_SIZE 131072 //128k
//#define MEM_SIZE 65536  //64k
  #define MEM_SIZE 32768  //32k; MKRZero, Feather M0
//#define MEM_SIZE 16384  //16k
//#define MEM_SIZE 8192   //8k;  Mega 2560
//#define MEM_SIZE 2048   //2k
//#define MEM_SIZE 512    //512

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
typedef uint16_t subnum;
typedef uint8_t  sublen;

//All Chika programs' memory
#define CHIKA_SIZE (uint32_t)(MEM_SIZE * .4)
//All subscription messages and program ID's
#define SUBS_SIZE  (strilen)(MEM_SIZE * 0.1)
#define MAX_SUBS   (subnum)(SUBS_SIZE / 16)

#define MIN_NUM_PROG 4
#define MAX_NUM_PROG 8
#define MAX_PROG_RAM (CHIKA_SIZE / MIN_NUM_PROG)


#ifdef SDCARD_SS_PIN
  #define SD_CARD_PIN SDCARD_SS_PIN
#endif
#ifndef SDCARD_SS_PIN
  #define SD_CARD_PIN 4
#endif


//NOTE: when adding a new value type or op longer than 1B
//  ensure its implementation in constByteLen
//  as this ensures the VM can skip it in ROM
enum IType : uint8_t {
  Form_Eval = 0x00,
  Form_If = 0x01, Form_Or = 0x02, Form_And = 0x03, Form_Case = 0x04,
  FORMS_END = 0x04,

  Val_True  = 0x05, Val_False = 0x06,
  Val_Str   = 0x07,
  Param_Val = 0x08,
  Bind_Mark = 0x09, //e.g. test=
  Bind_Val  = 0x0A, //     test
  XBind_Val = 0x0B, //     .test
  Val_Vec   = 0x0C,
  Val_Blob  = 0x0D,
  Val_U08   = 0x10,
  Val_U16   = 0x11,
  Val_I32   = 0x12,
  Val_Char  = 0x13,
  Val_Args  = 0x19,
  Var_Op    = 0x1A,
  Var_Func  = 0x1B,
  Val_Nil   = 0x1E,
  
  OPS_START = 0x22,
  Op_Func   = 0x22,
  Op_If     = 0x23, Op_Case   = 0x24,
  Op_Or     = 0x25, Op_And    = 0x26,
  Op_Not    = 0x27,
  Op_Return = 0x28, Op_Recur  = 0x29,
  Op_Equal  = 0x2A, Op_Equit  = 0x2B,
  Op_Nequal = 0x2C, Op_Nequit = 0x2D,
  Op_LT     = 0x2E, Op_LTE    = 0x2F,
  Op_GT     = 0x30, Op_GTE    = 0x31,
  Op_Add    = 0x32, Op_Sub    = 0x33,
  Op_Mult   = 0x34, Op_Div    = 0x35,
  Op_Mod    = 0x36, Op_Pow    = 0x37,
  Op_BNot   = 0x38, Op_BAnd   = 0x39,
  Op_BOr    = 0x3A, Op_BXor   = 0x3B,
  Op_LShift = 0x3C, Op_RShift = 0x3D,
  Op_PinMod = 0x3E,
  Op_DigIn  = 0x3F, Op_DigOut = 0x40,
  Op_AnaIn  = 0x41, Op_AnaOut = 0x42,
  Op_Read   = 0x43, Op_Write  = 0x44,
  Op_Append = 0x45, Op_Delete = 0x46,
  Op_Str    = 0x47, Op_Vec    = 0x48,
  Op_Nth    = 0x49, Op_Len    = 0x4A,
  Op_Sect   = 0x4B, Op_BSect  = 0x4C,
  Op_Burst  = 0x4D,
  Op_Reduce = 0x4E, Op_Map    = 0x4F,
  Op_For    = 0x50, Op_Loop   = 0x51,
  Op_Binds  = 0x52,
  Op_Val    = 0x53, Op_Do     = 0x54,
  Op_MPub   = 0x55, Op_MSub   = 0x56,
  Op_MUnsub = 0x57,
  Op_Type   = 0x58, Op_Cast   = 0x59,
  Op_MsNow  = 0x5A,
  Op_Sleep  = 0x5B,
  Op_Print  = 0x5C,
  Op_Debug  = 0x5D,
  Op_Load   = 0x5E,
  Op_Comp   = 0x5F,
  Op_Halt   = 0x60,
  Op_Bind   = 0x61,
  Op_Param  = 0x62
};

itemlen constByteLen (IType, uint8_t* = nullptr);
IType fitInt (uint8_t);
