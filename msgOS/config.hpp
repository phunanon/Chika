#pragma once
#include <stdint.h>
#include "string.h"

//WORK_SIZE: space reserved for Arduino sketch memory
  #define WORK_SIZE 8192 //8k
//#define WORK_SIZE 1536 //1.5k
//#define WORK_SIZE 1024 //1k

//MEM_SIZE: Arduino SRAM size
//#define MEM_SIZE 262144 //256k
//#define MEM_SIZE 131072 //128k
//#define MEM_SIZE 65536  //64k
  #define MEM_SIZE 32768  //32k
//#define MEM_SIZE 16384  //16k
//#define MEM_SIZE 8192   //8k
//#define MEM_SIZE 2048   //2k
//#define MEM_SIZE 512    //512

//CHIKA_SIZE: all Chika programs' memory
#define CHIKA_SIZE (MEM_SIZE - WORK_SIZE)

#define NUM_PROG     4
#define MAX_PROG_RAM 8192


typedef uint32_t memolen;
typedef uint16_t proglen;
typedef uint16_t itemlen;
typedef uint8_t  prognum;
typedef uint16_t itemnum;
typedef uint32_t bytenum;
typedef uint16_t funcnum;
typedef uint16_t funclen;
typedef uint8_t  argnum;
typedef uint16_t varnum;
typedef uint16_t strilen;
typedef uint16_t vectlen;


//NOTE: when adding a new value type or op longer than 1B
//  ensure its implementation in constByteLen
enum IType : uint8_t {
  Form_Eval = 0x00, Form_If = 0x01, Form_Or = 0x02, Form_And = 0x03,
  FORMS_END = 0x03,

  Val_True  = 0x04, Val_False = 0x05,
  Val_Str   = 0x06,
  Param_Val = 0x07,
  Bind_Var  = 0x08,
  Var_Val   = 0x09,
  Val_Vec   = 0x0A, Val_Dict  = 0x0B,
  Val_U08   = 0x10,
  Val_U16   = 0x11,
  Val_I32   = 0x12,
  Val_Char  = 0x19,
  Var_Op    = 0x1A,
  Var_Func  = 0x1B,
  Val_Nil   = 0x1E,

  OPS_START = 0x1F,
  Op_Func   = 0x22,
  Op_If     = 0x23,
  Op_Or     = 0x24,
  Op_And    = 0x25,
  Op_Var    = 0x2A,
  Op_Param  = 0x2B,
  Op_Recur  = 0x2F,
  Op_Equal  = 0x30, Op_Equit  = 0x31,
  Op_LT     = 0x32, Op_LTE    = 0x33,
  Op_GT     = 0x34, Op_GTE    = 0x35,
  Op_Add    = 0x36,
  Op_Sub    = 0x37,
  Op_Mult   = 0x38,
  Op_Div    = 0x39,
  Op_Mod    = 0x3A,
  Op_Str    = 0x44,
  Op_Type   = 0xAA,
  Op_Cast   = 0xAB,
  Op_Vec    = 0xB0,
  Op_Nth    = 0xB1,
  Op_Len    = 0xB2,
  Op_Sect   = 0xB3,
  Op_Burst  = 0xBA,
  Op_Reduce = 0xBB,
  Op_Map    = 0xBC,
  Op_Val    = 0xCD, Op_Do     = 0xCE,
  Op_MsNow  = 0xE0,
  Op_Print  = 0xEE
};

funclen constByteLen (IType, uint8_t* = nullptr);
IType fitInt (uint8_t);