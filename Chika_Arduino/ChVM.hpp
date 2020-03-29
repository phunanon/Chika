#pragma once
#include "string.h"
#include "config.hpp"
#include "utils.hpp"
#include "Item.hpp"
#include "ChVM_Harness.hpp"
#include "Broker.hpp"

class Broker;

struct ProgInfo {
  bytenum  memLen;  //Length of the program ROM, RAM, and other data
  proglen  romLen;  //Length of the program
  itemnum  numItem; //Number of LIFO items
  bytenum  numByte; //Number of LIFO bytes
  uint32_t sleepUntil; //Time in miliseconds to not heartbeat until
  bool     isHalting = false;
  ProgInfo () {}
  ProgInfo (bytenum _memLen, proglen _romLen, itemnum _numItem, bytenum _numByte)
    : memLen(_memLen), romLen(_romLen), numItem(_numItem), numByte(_numByte) {}
};

class ChVM {
  uint8_t   mem[CHIKA_SIZE]; //All programs' memory
  ProgInfo  progs[MAX_NUM_PROG]; //Program descriptors
  ChVM_Harness* harness;
  Broker    broker;

  bool heartbeat (prognum);

  bytenum   memOffset (prognum);  //Calculate offset of a program's memory
  void      numItem   (itemnum);  //
  itemnum   numItem   ();         // Set/Get number of LIFO items
  void      numByte   (bytenum);  //
  bytenum   numByte   ();         // Set/Get number of LIFO bytes

  //Program item LIFO
  itemlen  itemsBytesLen (itemnum, itemnum); //Number of bytes on LIFO byte stack between two items
  Item*    i          (itemnum);  //Traverse items
  uint8_t* iBytes     (itemnum);   //Access item bytes directly
  Item*    iLast      ();         //Access last item

  int32_t  iInt       (itemnum);  //Item-agnostic readNum
  bool     iBool      (itemnum);  //Item-agnostic truth test
  bool     iCBool     (itemnum);  //Item-agnostic C-like truth test
  const char* iStr    (itemnum);  //Item-agnostic string

  void     trunStack  (itemnum);
  uint8_t* stackItem  ();
  void     stackItem  (Item*);
  void     stackItem  (Item);
  void     returnItem (itemnum, Item*);
  void     returnItem (itemnum, Item);
  void     returnCollapseLast (itemnum);           //
  void     returnCollapseItem (itemnum, Item*);    //
  void     returnCollapseItem (itemnum, Item);     // Collapse the LIFO so the last stack item is moved to be the return item
  void     returnItemFrom     (itemnum, itemnum);
  void     restackCopy        (itemnum, itemnum);
  void     returnNil  (itemnum);
  void     returnBool (itemnum, bool);
  void     returnInt  (itemnum, int32_t, uint8_t);
  void     stackNil   ();
  void     iPop       ();                   //Pop item
  void     collapseItems (itemnum, itemnum);

  void     purgeProg (prognum);
  uint8_t* pFunc     (funcnum);
  bool     findBind  (itemnum&, bindnum, bool);

  void     collapseArgs  (itemnum&);
  void     tailCallOptim (IType, itemnum&);
  //All may leave multiple items on the stack
  void     burstItem ();
  void     op_Sect   (itemnum, bool);
  //All leave one item on the stack
  bool     exeFunc   (funcnum, itemnum);
  void     exeForm   ();
  void     nativeOp  (IType, itemnum);
  vectlen  vectLen   (itemnum);
  void     op_Not    (itemnum);
  void     op_Equal  (itemnum, bool);
  void     op_Diff   (itemnum, IType);
  void     op_Arith  (itemnum, IType);
  void     op_GPIO   (itemnum, IType);
  void     op_Read   (itemnum);
  void     op_Write  (itemnum);
  void     op_Append (itemnum);
  void     op_Delete (itemnum);
  void     op_Str    (itemnum);
  void     op_Type   (itemnum);
  void     op_Cast   (itemnum);
  void     op_Vec    (itemnum);
  void     op_Nth    (itemnum);
  void     op_Len    (itemnum);
  void     op_Reduce (itemnum);
  void     op_Map    (itemnum);
  void     op_For    (itemnum);
  void     op_Loop   (itemnum);
  void     op_Binds  (itemnum);
  void     op_Val    (itemnum);
  void     op_Do     (itemnum);
  void     op_Pub    (itemnum);
  void     op_Sub    (itemnum);
  void     op_Unsub  (itemnum);
  void     op_MsNow  (itemnum);
  void     op_Sleep  (itemnum);
  void     op_Print  (itemnum);
  void     op_Debug  (itemnum);
  void     op_Load   (itemnum);
  //Returns nothing
  void     op_Halt   ();

public:
  ChVM (ChVM_Harness*);
  void entry     ();
  bool heartbeat ();
  void msgInvoker(prognum, funcnum, const char*, Item*, uint8_t*, bool);

  prognum  numProg;           //Number of loaded progs
  uint8_t* getPROM ();
  void     switchToProg (prognum, proglen = 0, bytenum = 0); //Set current program, and optionally set its ROM + RAM lengths
};
