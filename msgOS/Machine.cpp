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
void Machine::returnItem (itemnum replace, Item desc) {
  memcpy(i(replace), &desc, sizeof(Item));
  numItem(replace + 1);
  numByte(itemsBytesLen(0, replace + 1));
}
void Machine::returnCollapseItem (itemnum replace, Item desc) {
  memmove(returnItem(replace), stackItem(), desc.len);
  returnItem(replace, desc);
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

void Machine::exeFunc (funcnum fNum, itemnum firstParamItem) {
debugger("exeFunc", true, fNum);
  uint8_t* f = pFunc(fNum);
  f += sizeof(funcnum);
  funclen fLen = *(funclen*)f;
  f += sizeof(funclen);
debugger("func len:", true, fLen);
  uint8_t* fEnd = f + fLen;
  while (f != fEnd) {
    ++f; //Skip next form header
    f = exeForm(f, firstParamItem);
debugger("f   :", true, (intptr_t)f);
debugger("fEnd:", true, (intptr_t)fEnd);
debugger("form done", false, 0);
    if (f != fEnd)
      iPop();
  }
}

uint8_t* Machine::exeForm (uint8_t* f, itemnum firstParamItem) {
  itemnum firstArgItem = numItem();
  while (true) {
//delay(1000);
printMem(f, 16);
/*for (itemnum it = 0; it < numItem(); ++it) {
  debugger("item", true, it);
  debugger("  len: ", true, i(it)->len);
  debugger("  type:", true, i(it)->type());
  debugger("  kind:", true, i(it)->kind());
  debugger("", false, 0);
}*/
printItems(pItems(), numItem());
    IType type = (IType)*f;
    if (type == Mark_Form) { //Form?
debugger("Found form @", true, (intptr_t)f);
      ++f; //Skip form header
      f = exeForm(f, firstParamItem);
    } else
    if (*f == Mark_Arg) { //Arg (Reference)?
      itemnum iNum = firstParamItem + *(++f);
      Item item = Item(sizeof(itemnum), i(iNum)->type(), Reference);
      *(itemnum*)stackItem() = iNum;
      stackItem(item);
      f += sizeof(argnum);
    } else
    if (*f < _ValsUntil) { //Constant?
debugger("Found const @", true, f - pROM());
debugger("  type:", true, *f);
      Item item = Item(constByteLen(type, ++f), type, Constant);
      //++f; //Skip type code
      //*(proglen*)stackItem() = f - pROM();
proglen constPos = f - pROM();
uint8_t* s = stackItem();
memcpy(stackItem(), &constPos, sizeof(proglen));
      stackItem(item);
debugger("  pos:", true, constPos);
debugger("  len:", true, item.len);
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

void Machine::nativeOp (IType op, itemnum firstParamItem) {
  switch (op) {
    case Op_Add:   op_Add  (firstParamItem); break;
    case Op_Str:   op_Str  (firstParamItem); break;
    case Op_Print: op_Str  (firstParamItem);
                   op_Print(firstParamItem); break;
    default: break;
  }
}

void Machine::op_Add (itemnum firstParamItem) {
  IType type = i(firstParamItem)->type();
  itemlen len = constByteLen(type);
  uint8_t* result = stackItem();
  int32_t sum = 0;
debugger("ADDING type", true, type);
  for (itemnum it = firstParamItem, itEnd = numItem(); it < itEnd; ++it)
    sum += readNum(iData(it), min(len, constByteLen(i(it)->type())));
  writeNum(result, sum, len);
  returnCollapseItem(firstParamItem, Item(len, type, Value));
}

void Machine::op_Str (itemnum firstParamItem) {
  uint8_t* result = stackItem();
  strilen len = 0;
  for (itemnum it = firstParamItem, itEnd = numItem(); it < itEnd; ++it) {
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
  returnCollapseItem(firstParamItem, Item(len, Val_Str, Value));
}

void Machine::op_Print (itemnum firstParamItem) {
  if (i(firstParamItem)->type() == Val_Str)
    debugger((const char*)iData(firstParamItem), false, 0);
  else
    debugger("Must be string.", false, 0);
  returnNil(firstParamItem);
}
