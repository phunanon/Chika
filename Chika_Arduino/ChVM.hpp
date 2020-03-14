#pragma once
#include "string.h"
#include "config.hpp"
#include "utils.hpp"
#include "Item.hpp"
#include "ChVM_Harness.hpp"

struct ProgInfo {
  proglen romLen;
  bytenum ramLen;
  itemnum numItem;
  bytenum numByte;
  ProgInfo () {}
  ProgInfo (proglen _romLen, bytenum _ramLen, itemnum _numItem, bytenum _numByte)
    : romLen(_romLen), ramLen(_ramLen), numItem(_numItem), numByte(_numByte) {}
};

extern ProgInfo progs[];

class ChVM {
  prognum   pNum;
  uint8_t*  pBytes;
  uint8_t*  pFirstItem;
  ProgInfo* pInfo;
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
  bool     iBool      (itemnum);  //Item-agnostic truth test
  bool     iCBool     (itemnum);  //Item-agnostic C-like truth test

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
  void     stackNil   ();
  void     iPop       ();                   //Pop item
  void     collapseItems (itemnum, itemnum);

  uint8_t* pFunc     (funcnum);
  bool     findBind  (itemnum&, bindnum);

  void     collapseArgs  (itemnum&);
  void     tailCallOptim (IType, itemnum&);
  void     burstItem ();
  void     op_Sect   (itemnum, bool);
  //All leave one V item on the stack
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
  void     op_Val    (itemnum);
  void     op_Do     (itemnum);
  void     op_MsNow  (itemnum);
  void     op_Print  (itemnum);
  void     op_Debug  (itemnum);

public:
  ChVM ();
  void entry ();
  bool heartbeat (prognum);

  uint8_t* mem;
  //Program ROM
  uint8_t* pROM;
  void     romLen (proglen);            //Set length of program ROM
  void     setPNum (prognum);

  ChVM_Harness* harness;
};
