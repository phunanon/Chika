#include "Machine.hpp"

Item::Item (itemlen _len, IType type, IKind kind) {
  len = _len;
  typeAndKind = (uint8_t)(kind << 6) | (uint8_t)(type & 0x3F);
}
IType Item::type () {
  return (IType)(typeAndKind & 0x3F);
}
IKind Item::kind () {
  return (IKind)(typeAndKind >> 6);
}

Machine::Machine () {}

uint8_t* Machine::pHead () {
  return mem + (pNum * progSize);
}
uint8_t* Machine::pROM () {
  return pHead() + sizeof(ProgInfo);
}
uint8_t* Machine::pBytes () {
  return pROM() + romLen();
}
uint8_t* Machine::pItems () {
  return pHead() + progSize;
}
ProgInfo* Machine::pInfo () {
  return (ProgInfo*)pHead();
}

void Machine::romLen (proglen len) {
  pInfo()->len = len;
}
proglen Machine::romLen () {
  return pInfo()->len;
}
void Machine::numItem (itemnum n) {
  pInfo()->numItem = n;
}
itemnum Machine::numItem () {
  return pInfo()->numItem;
}
void Machine::numByte (bytenum n) {
  pInfo()->numByte = n;
}
bytenum Machine::numByte () {
  return pInfo()->numByte;
}

void Machine::p (proglen pByte, uint8_t b) {
  *(pROM() + pByte) = b;
}
uint8_t Machine::p (proglen pByte) {
  return *(pROM() + pByte);
}

itemlen Machine::itemBytesLen (Item* it) {
  return it->kind() == Value ? it->len : 2;
}
//`to` is exclusive
itemlen Machine::itemsBytesLen (itemnum from, itemnum to) {
  bytenum n = 0;
  for (itemnum it = from; it < to; ++it)
    n += itemBytesLen(i(it));
  return n;
}
Item* Machine::i (itemnum iNum) {
  return (Item*)(pItems() - ((iNum+1) * sizeof(Item)));
}
uint8_t* Machine::iBytes (itemnum iNum) {
  return pBytes() + itemsBytesLen(0, iNum);
}
uint8_t* Machine::iData (itemnum iNum) {
  IKind kind = i(iNum)->kind();
  uint8_t* bPtr = iBytes(iNum);
  if (kind == Constant) {
    return pROM() + (*(proglen*)bPtr);
  }
  if (kind == Reference)
    return iData(*(itemnum*)bPtr);
  if (kind == Value)
    return bPtr;
}
Item* Machine::iLast () {
  itemnum nItem = numItem();
  if (!nItem) return nullptr;
  return i(nItem - 1);
}

int32_t Machine::iInt (itemnum iNum) {
  return readNum(iData(iNum), constByteLen(i(iNum)->type()));
}

uint8_t* Machine::stackItem () {
  return pBytes() + numByte();
}
void Machine::stackItem (Item desc) {
  numItem(numItem() + 1);
  numByte(numByte() + itemBytesLen(&desc));
  memcpy(iLast(), &desc, sizeof(Item));
}
uint8_t* Machine::returnItem (itemnum replace) {
  return pBytes() + itemsBytesLen(0, replace);
}
void Machine::returnItem (itemnum replace, Item* desc) {
  memcpy(i(replace), desc, sizeof(Item));
  numItem(replace + 1);
  numByte(itemsBytesLen(0, replace + 1));
memset(stackItem(), 0, (uint8_t*)iLast() - stackItem());
}
void Machine::returnCollapseItem (itemnum replace, Item* desc) {
  memmove(returnItem(replace), stackItem(), desc->len);
  returnItem(replace, desc);
}
void Machine::returnItem (itemnum replace, Item desc) {
  returnItem(replace, &desc);
}
void Machine::returnCollapseItem (itemnum replace, Item desc) {
  returnCollapseItem(replace, &desc);
}
void Machine::returnNil (itemnum replace) {
  returnItem(replace, Item(0, Val_Nil, Value));
}

void Machine::iPop (itemnum n = 1) {
  itemnum nItem = numItem();
  numItem(nItem - n);
  numByte(numByte() - i(nItem - 1)->len);
}




void Machine::heartbeat (prognum _pNum) {
  pNum = _pNum;
  exeFunc(0x0000, -1);
  //Discard heartbeat function result
  iPop();
debugger("badum", false, 0);
while (true);
}

uint8_t* Machine::pFunc (funcnum fNum) {
  uint8_t* r = pROM();
  uint8_t* rEnd = pBytes();
  while (r != rEnd) {
    if (*(proglen*)r == fNum)
      return r;
    r += *(proglen*)(r + 1);
  }
debugger("func not found", false, 0);
}

void Machine::exeFunc (funcnum fNum, itemnum firstParam) {
debugger("exeFunc", true, fNum);
  uint8_t* f = pFunc(fNum);
  f += sizeof(funcnum);
  funclen fLen = *(funclen*)f;
  f += sizeof(funclen);
debugger("func len:", true, fLen);
  uint8_t* fEnd = f + fLen;
  while (f != fEnd) {
    ++f; //Skip next form header
    f = exeForm(f, firstParam);
debugger("f   :", true, (intptr_t)f);
debugger("fEnd:", true, (intptr_t)fEnd);
debugger("form done", false, 0);
    if (f != fEnd)
      iPop();
  }
}

uint8_t* Machine::exeForm (uint8_t* f, itemnum firstParam) {
  itemnum firstArgItem = numItem();
  while (true) {
//delay(1000);
printMem(f, 24);
printItems(pItems(), numItem());
    IType type = (IType)*f;
    if (type == Mark_Form) { //Form?
debugger("Found form @", true, (intptr_t)f);
      ++f; //Skip form header
      f = exeForm(f, firstParam);
    } else
    if (*f == Mark_Arg) { //Arg (Reference)?
      itemnum iNum = firstParam + *(++f);
      Item item = Item(sizeof(itemnum), i(iNum)->type(), Reference);
      *(itemnum*)stackItem() = iNum;
      stackItem(item);
      f += sizeof(argnum);
    } else
    if (*f < _ValsUntil) { //Constant?
debugger("Found const @", true, (f+1) - pROM());
debugger("  type:", true, *f);
      Item item = Item(constByteLen(type, ++f), type, Constant);
      //++f; //Skip type code
      //*(proglen*)stackItem() = f - pROM();
proglen constPos = f - pROM();
uint8_t* s = stackItem();
memcpy(stackItem(), &constPos, sizeof(proglen));
      stackItem(item);
debugger("  len:", true, item.len);
debugger("  byte:", true, *f);
      f += constByteLen(type, f);
    } else
    if (*f == Op_Func) { //Func?
      funcnum fNum = *(funcnum*)(++f);
      exeFunc(fNum, firstArgItem);
      ++f;
      break;
    } else { //Native op?
debugger("OP:", true, *f);
      nativeOp((IType)*f, firstArgItem);
      ++f; //Skip op code
      break;
    }
  }
  return f;
}

void Machine::nativeOp (IType op, itemnum firstParam) {
  switch (op) {
    case Op_Add:   op_Add  (firstParam); break;
    case Op_Str:   op_Str  (firstParam); break;
    case Op_Print: op_Print(firstParam); break;
    case Op_Vec:   op_Vec  (firstParam); break;
    case Op_Nth:   op_Nth  (firstParam); break;
    default: break;
  }
}

void Machine::op_Add (itemnum firstParam) {
  IType type = i(firstParam)->type();
  itemlen len = constByteLen(type);
  uint8_t* result = stackItem();
  int32_t sum = 0;
debugger("ADDING type", true, type);
  for (itemnum it = firstParam, itEnd = numItem(); it < itEnd; ++it)
    sum += readNum(iData(it), min(len, constByteLen(i(it)->type())));
  writeNum(result, sum, len);
  returnCollapseItem(firstParam, Item(len, type, Value));
}

void Machine::op_Str (itemnum firstParam) {
  uint8_t* result = stackItem();
  strilen len = 0;
  for (itemnum it = firstParam, itEnd = numItem(); it < itEnd; ++it) {
    Item* item = i(it);
    uint8_t* target = result + len;
    IType type = item->type();
    switch (type) {
      case Val_Str:
        memcpy(target, iData(it), item->len - 1);
        len += item->len - 1;
        break;
      case Val_Byte:
      case Val_Word:
      case Val_Int:
        len += int2chars(readNum(iData(it), constByteLen(type)), target);
        break;
    }
  }
  result[len++] = 0;
  returnCollapseItem(firstParam, Item(len, Val_Str, Value));
}

void Machine::op_Print (itemnum firstParam) {
  op_Str(firstParam);
  debugger((const char*)iData(firstParam), false, 0);
  returnNil(firstParam);
}

void Machine::op_Vec (itemnum firstParam) {
debugger("\n   VECTORISING\n", false, 0);
  //Copy item descriptors onto the end of the byte stack
  uint8_t* descs = stackItem();
  vectlen nItems = (numItem() - firstParam);
  bytenum itemsLen = sizeof(Item) * nItems;
  memcpy(descs, iLast(), itemsLen);
  //Append number of items
  writeNum(descs + itemsLen, nItems, sizeof(vectlen));
  //Return umbrella item descriptor
  bytenum bytesLen = itemsBytesLen(firstParam, numItem()) + itemsLen + sizeof(vectlen);
  returnItem(firstParam, Item(bytesLen, Val_Vec, Value));
}

void Machine::op_Nth (itemnum firstParam) {
  int32_t nth = iInt(firstParam);
  //Collate vector info
  itemnum iVec = firstParam + 1;
  uint8_t* vBytes = iBytes(iVec);
  uint8_t* vEnd = (vBytes + i(iVec)->len) - sizeof(vectlen);
  itemnum vNumItem = readNum(vEnd, sizeof(vectlen));
  Item* vItems = &((Item*)vEnd)[-vNumItem];
  //Find item descriptor at nth
  Item* nthItem = &((Item*)vEnd)[-(nth + 1)];
  //Copy bytes into return position
  uint8_t* itemBytes = vBytes;
  for (itemnum vi = 0; vi < nth; ++vi)
    itemBytes += itemBytesLen(&vItems[vi]);
  memcpy(stackItem(), itemBytes, itemBytesLen(nthItem));
  //Return nth item descriptor
  returnCollapseItem(firstParam, nthItem);
}
