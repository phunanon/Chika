#pragma once
#include "string.h"
#include "config.hpp"
#include "utils.hpp"
#include "Item.hpp"

/*
Prog memory:
-- prog HEAD
- two bytes: uint16_t progLen
- two bytes: uint16_t item stack count
- four bytes: uint32_t item stack byte count
-- prog ROM
- onward: progLen bytes: program ROM
-- prog LIFO bytes
- onward: N bytes: program item LIFO bytes
-- prog LIFO items
- tail backward: N bytes: program item LIFO items
*/

struct __attribute__((__packed__)) ProgInfo {
  proglen len;
  itemnum numItem;
  bytenum numByte;

  ProgInfo (proglen _len, itemnum _numItem, bytenum _numByte)
    : len(_len), numItem(_numItem), numByte(_numByte) {}
};


class Machine {
  uint8_t*  pHead    ();
  uint8_t*  pBytes   ();
  uint8_t*  pItems   ();
  ProgInfo* pInfo    ();
  proglen   romLen   ();           // Get length of program ROM
  void      numItem  (itemnum);  //
  itemnum   numItem  ();           // Set/Get number of LIFO items
  void      numByte  (bytenum);  //
  bytenum   numByte  ();           // Set/Get number of LIFO bytes

  //Program item LIFO
  itemlen  itemBytesLen  (Item*);          //Number of bytes on LIFO byte stack for an item
  itemlen  itemsBytesLen (itemnum, itemnum); //Number of bytes on LIFO byte stack between two items
  Item*    i          (itemnum);  //Traverse items
  uint8_t* iBytes     (itemnum);   //Access item bytes directly
  uint8_t* iData      (itemnum);   //Access item bytes, traversing references
  Item*    iLast      ();         //Access last item

  int32_t  iInt       (itemnum);  //Item-agnostic readNum

  uint8_t* stackItem  ();
  void     stackItem  (Item*);
  void     stackItem  (Item);
  void     setStackN  (itemnum);
  void     returnItem (itemnum, Item*);
  void     returnItem (itemnum, Item);
  void     returnCollapseLast (itemnum);           //
  void     returnCollapseItem (itemnum, Item*);    //
  void     returnCollapseItem (itemnum, Item);     // Collapse the LIFO so the last stack item is moved to be the return item
  void     returnItemFrom     (itemnum, itemnum);
  void     returnNil  (itemnum);
  void     iPop       (itemnum);                   //Pop items

  uint8_t* pFunc     (funcnum);

  //All leave one V item on the stack
  void     exeFunc   (funcnum, itemnum);
  uint8_t* exeForm   (uint8_t*, itemnum);
  void     nativeOp  (IType, itemnum);
  void     op_Equal  (itemnum, bool);
  void     op_Diff   (itemnum, IType);
  void     op_Arith  (itemnum, IType);
  void     op_Str    (itemnum);
  void     op_Vec    (itemnum);
  void     op_Nth    (itemnum);
  void     op_Len    (itemnum);
  void     op_Val    (itemnum);
  void     op_Do     (itemnum);
  void     op_MsNow  (itemnum);
  void     op_Print  (itemnum);
public:
  prognum pNum;
  Machine ();
  void heartbeat (prognum);

  uint8_t* mem;
  memolen  progSize;
  //Program ROM
  uint8_t* pROM   ();
  void     romLen (proglen);            //Set length of program ROM

  //TODO make a class of these
  void     (*debugger)   (const char*, bool, uint32_t);
  void     (*printMem)   (uint8_t*, uint8_t);
  void     (*printItems) (uint8_t*, itemnum);
  uint8_t  (*loadProg)   (const char*);
  uint8_t  (*unloadProg) (const char*);
  uint32_t (*msNow)      ();
};
