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


typedef uint32_t memolen;
typedef uint16_t proglen;
typedef uint16_t itemlen;
typedef uint8_t  prognum;
typedef uint16_t itemnum;
typedef uint32_t bytenum;
typedef uint16_t funcnum;
typedef uint16_t funclen;
typedef uint8_t  argnum;
typedef uint16_t strilen;
typedef uint16_t vectlen;


#define _ValsUntil 0x22 //(exclusive)
//TODO: refactor ints to e.g. Val_U08
enum IType {
  Mark_Form = 0x00,
  Val_Str   = 0x01,
  Mark_Arg  = 0x06,
  Val_Vec   = 0x0A,
  Val_Byte  = 0x10, // u08
  Val_Word  = 0x11, // u16
  Val_Int   = 0x12, // i32
  Val_Long  = 0x13, // i64
  Val_Deci  = 0x14, // i64 *_DeciPrecision
  Val_End   = 0x20,
  Val_Nil   = 0x21,
  ///
  Op_Func   = 0x22,
  Op_Add    = 0x33,
  Op_Str    = 0x44,
  Op_Vec    = 0xBB,
  Op_Nth    = 0xCC,
  Op_Print  = 0xEE
};

funclen constByteLen (IType t, uint8_t* body = nullptr);
