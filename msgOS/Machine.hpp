#pragma once
#include "string.h"
#include "config.hpp"
#include "utils.hpp"


//Constant:  bytes are program position as proglen
//Reference: bytes are item position as itemnum
//Value:     bytes are raw value
enum IKind { Constant, Reference, Value };

class __attribute__((__packed__)) Item {
  uint8_t typeAndKind;
public:
  itemlen len; //Length of value or const/referenced value
  Item (itemlen, IType, IKind);
  IType type ();
  IKind kind ();
};

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
  uint8_t*  pROM     ();
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
  void     stackItem  (Item);
  uint8_t* returnItem (itemnum);
  void     returnItem (itemnum, Item*);
  void     returnItem (itemnum, Item);
  void     returnCollapseLast (itemnum);           //
  void     returnCollapseItem (itemnum, Item*);    //
  void     returnCollapseItem (itemnum, Item);     // Collapse the LIFO so the last stack item is moved to be the return item
  void     returnNil  (itemnum);
  void     iPop       (itemnum);                   //Pop items

  uint8_t* pFunc     (funcnum);

  //All leave one V item on the stack
  void     exeFunc   (funcnum, itemnum);
  uint8_t* exeForm   (uint8_t*, itemnum);
  void     nativeOp  (IType, itemnum);
  void     op_Add    (itemnum);
  void     op_Str    (itemnum);
  void     op_Print  (itemnum);
  void     op_Vec    (itemnum);
  void     op_Nth    (itemnum);
  void     op_Val    (itemnum);
public:
  prognum pNum;
  Machine ();
  void heartbeat (prognum);

  uint8_t* mem;
  memolen  progSize;
  //Program ROM
  void     romLen (proglen);            //Set length of program ROM
  void     p      (proglen, uint8_t);   //Write program byte
  uint8_t  p      (proglen);            //Read program byte

  void    (*debugger)   (const char*, bool, uint32_t);
  void    (*printMem)   (uint8_t*, uint8_t);
  void    (*printItems) (uint8_t*, itemnum);
  uint8_t (*loadProg)   (const char*);
  uint8_t (*unloadProg) (const char*);
  void    (*delay)      (long unsigned int);
};
