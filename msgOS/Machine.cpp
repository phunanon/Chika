#include "Machine.hpp"

Machine::Machine () {}

ProgInfo* Machine::pInfo () {
  return (ProgInfo*)pHead;
}

void Machine::romLen (proglen len) {
  pInfo()->len = len;
  pBytes = pROM + len;
}
proglen Machine::romLen () {
  return pInfo()->len;
}
void Machine::setPNum (prognum n) {
  pNum = n;
  pHead = mem + (pNum * progSize);
  pROM = pHead + sizeof(ProgInfo);
  pFirstItem = (pHead + progSize) - sizeof(Item);
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

itemlen Machine::itemBytesLen (Item* it) {
  return it->isConst() ? sizeof(proglen) : it->len;
}
//`to` is exclusive
itemlen Machine::itemsBytesLen (itemnum from, itemnum to) {
  bytenum n = 0;
  for (itemnum it = from; it < to; ++it)
    n += itemBytesLen(i(it));
  return n;
}
Item* Machine::i (itemnum iNum) {
  return (Item*)(pFirstItem - (iNum * sizeof(Item)));
}
uint8_t* Machine::iBytes (itemnum iNum) {
  return pBytes + itemsBytesLen(0, iNum);
}
uint8_t* Machine::iData (itemnum iNum) {
  uint8_t* bPtr = iBytes(iNum);
  if (i(iNum)->isConst())
    return pROM + (*(proglen*)bPtr);
  else
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
  return pBytes + numByte();
}
void Machine::stackItem (Item* desc) {
  numItem(numItem() + 1);
  numByte(numByte() + itemBytesLen(desc));
  memcpy(iLast(), desc, sizeof(Item));
}
void Machine::stackItem (Item desc) {
  stackItem(&desc);
}
void Machine::returnItem (itemnum replace, Item* desc) {
  bytenum newNumBytes = (numByte() - itemsBytesLen(replace, numItem())) + itemBytesLen(desc);
  //Replace item currently in the return position
  memcpy(i(replace), desc, sizeof(Item));
  //Update number of items and bytes
  numItem(replace + 1);
  numByte(newNumBytes);
}
void Machine::returnCollapseLast (itemnum replace) {
  itemlen lLen = itemBytesLen(iLast());
  //Move item bytes
  memmove(iBytes(replace), stackItem() - lLen, lLen);
  //Set old item in return position
  returnItem(replace, iLast());
}
void Machine::returnCollapseItem (itemnum replace, Item* desc) {
  //Move item bytes
  memmove(iBytes(replace), stackItem(), desc->len);
  //Set new item in return position
  returnItem(replace, desc);
}
void Machine::returnItem (itemnum replace, Item desc) {
  returnItem(replace, &desc);
}
void Machine::returnCollapseItem (itemnum replace, Item desc) {
  returnCollapseItem(replace, &desc);
}
void Machine::returnItemFrom (itemnum to, itemnum from) {
  Item* iFrom = i(from);
  //Copy bytes
  memmove(iBytes(to), iBytes(from), itemBytesLen(iFrom));
  //Copy descriptor
  returnItem(to, iFrom);
}
void Machine::returnNil (itemnum replace) {
  returnItem(replace, Item(0, Val_Nil));
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
}

uint8_t* Machine::pFunc (funcnum fNum) {
  uint8_t* r = pROM;
  uint8_t* rEnd = pBytes;
  while (r != rEnd) {
    if (*(funcnum*)r == fNum)
      return r;
    r += sizeof(funcnum);
    r += *(proglen*)r + sizeof(proglen);
  }
  return pROM;
}

bool Machine::findVar (itemnum& it, varnum vNum) {
  bool found = false;
  it = numItem() - 2; //Start -1 from last item, to permit "a= (inc a)"
  for (; ; --it) {
    if (i(it)->type() != Bind_Var) continue;
    //Test if this bind is the correct number
    if (*(varnum*)iData(it) == vNum) {
      found = true;
      break;
    }
    if (!it) break;
  }
  //The item after the bind is the variable
  return found ? ++it : false;
}

void Machine::exeFunc (funcnum fNum, itemnum firstParam) {
  uint8_t* f = pFunc(fNum);
  f += sizeof(funcnum);
  funclen fLen = *(funclen*)f;
  f += sizeof(funclen);
  uint8_t* fEnd = f + fLen;
  while (f != fEnd) {
    f = exeForm(f, firstParam);
    if (f != fEnd)
      iPop();
  }
  //Move last item into return position
  if (firstParam != (itemnum)-1)
    returnCollapseLast(firstParam);
}

enum IfResult : uint8_t { UnEvaled, WasTrue, WasFalse };

uint8_t* Machine::exeForm (uint8_t* f, itemnum firstParam) {
  IType formCode = *(IType*)f;
  ++f; //Skip form code
  itemnum firstArgItem = numItem();
  IfResult ifResult = UnEvaled;
  while (true) {

    //If we're in a special form
    if (formCode != Form_Eval) {
      //Special form logic
      switch (formCode) {
        case Form_If:
          //(if cond ...)
          //   ^
          if (firstArgItem == numItem()) break; //Nothing evaluted yet
          //(if true if-true[ if-false])
          //                ^
          if (ifResult == WasTrue) {
            //Was true: skip the if-false arg if present
            if (*f != Op_If) skipArg(&f);
            return ++f; //Skip if op
          } else
          //(if false if-true if-false)
          //                          ^
          if (ifResult == WasFalse) {
            return ++f; //Skip if op
          } else
          //(if cond if-true[ if-false])
          //        ^
          if (firstArgItem + 1 == numItem()) {
            if (!isTypeTruthy(iLast()->type())) {
              //False: skip the if-true arg
              skipArg(&f);
              //If there's no if-false, return nil
              if (*f == Op_If) {
                returnNil(firstArgItem);
                return ++f;
              }
              ifResult = WasFalse;
            } else ifResult = WasTrue;
            iPop(); //Forget condition item
          }
          break;

        case Form_Or:
          //Did or-form end without truthy value?
          if (*f == Op_Or) {
            returnNil(firstArgItem);
            return ++f;
          }
          if (firstArgItem == numItem()) break; //Nothing evaluted yet
          //If previous eval was false
          if (!isTypeTruthy(iLast()->type()))
            iPop(); //Forget condition item
          //Previous eval was true
          else {
            //Skip all args until Op_Or
            while (*f != Op_Or)
              skipArg(&f);
            return ++f;
          }
          break;

        case Form_And:
          bool evaled = firstArgItem != numItem();
          if (evaled) {
            //Test previous eval
            bool isTruthy = isTypeTruthy(iLast()->type());
            iPop(); //Forget condition item
            //If falsey
            if (!isTruthy) {
              //Skip all args until Op_And
              while (*f != Op_And)
                skipArg(&f);
              returnItem(firstArgItem, Item(0, Val_False));
              return ++f;
            }
          }
          //Did and-form end without a falsey value?
          if (*f == Op_And) {
            returnItem(firstArgItem, Item(0, evaled ? Val_True : Val_False));
            return ++f;
          }
          break;
      }
    }

    //Evaluate next
    IType type = (IType)*f;
    //If a form
    if (type <= FORMS_END) {
      f = exeForm(f, firstParam);
    } else
    //If a parameter
    if (type == Param_Val) {
      itemnum iNum = firstParam + *(argnum*)(++f);
      f += sizeof(argnum);
      Item* iArg = i(iNum);
      //Copy the referenced value to the stack top
      memcpy(stackItem(), iData(iNum), iArg->len);
      //Copy its item descriptor too, as value
      stackItem(Item(iArg->len, iArg->type()));
    } else
    //If a variable
    if (type == Var_Val) {
      varnum vNum = *(varnum*)(++f);
      f += sizeof(varnum);
      itemnum it;
      //If the variable is found
      if (findVar(it, vNum)) {
        Item* vItem = i(it);
        //Copy the bytes to the stack top
        memcpy(stackItem(), iBytes(it), vItem->len);
        //Copy its item descriptor too
        stackItem(vItem);
      } else
      //No variable found - return nil
        stackItem(Item(0, Val_Nil));
    } else
    //If a constant
    if (type < OPS_START) {
      Item item = Item(constByteLen(type, ++f), type, true);
      *(proglen*)stackItem() = f - pROM;
      stackItem(item);
      f += constByteLen(type, f);
    } else
    //If a program function
    if (type == Op_Func) {
      funcnum fNum = *(funcnum*)(++f);
      exeFunc(fNum, firstArgItem);
      f += sizeof(funcnum);
      break;
    } else
    //If a native op or function through a variable or parameter
    if (type == Op_Var || type == Op_Param) {
      itemnum it;
      bool found = true;

      if (type == Op_Var) {
        found = findVar(it, *(varnum*)(++f));
        f += sizeof(varnum);
      } else {
        it = firstParam + *(argnum*)(++f);
        f += sizeof(argnum);
      }
      
      if (!found)
      //Variable wasn't found
        returnNil(firstArgItem);
      else {
        IType type = i(it)->type();
        //If a native op
        if (type == Var_Op)
          nativeOp(*(IType*)iData(it), firstArgItem);
        else
        //If a program function
        if (type == Var_Func)
          exeFunc(*(funcnum*)iData(it), firstArgItem);
        else
        //Variable wasn't of Var_Op/Var_Func type
          returnNil(firstArgItem);
      }
      break;
    } else
    //If a native op
    {
      nativeOp(*(IType*)f, firstArgItem);
      ++f; //Skip op code
      break;
    }
  }
  return f;
}

void Machine::nativeOp (IType op, itemnum firstParam) {
  switch (op) {
    case Op_Equal: op_Equal(firstParam, true);  break;
    case Op_Equit: op_Equal(firstParam, false); break;
    case Op_GT: case Op_GTE: case Op_LT: case Op_LTE:
      op_Diff(firstParam, op); break;
    case Op_Add: case Op_Sub: case Op_Mult: case Op_Div: case Op_Mod:
      op_Arith(firstParam, op); break;
    case Op_Str:    op_Str   (firstParam); break;
    case Op_Vec:    op_Vec   (firstParam); break;
    case Op_Nth:    op_Nth   (firstParam); break;
    case Op_Len:    op_Len   (firstParam); break;
    case Op_Apply: case Op_Reduce:
      op_AppRed(firstParam, op == Op_Reduce); break;
    case Op_Val:    op_Val   (firstParam); break;
    case Op_Do:     op_Do    (firstParam); break;
    case Op_MsNow:  op_MsNow (firstParam); break;
    case Op_Print:  op_Print (firstParam); break;
    default: break;
  }
}


void Machine::burstVec () {
  itemnum iVec = numItem() - 1;
  uint8_t* vBytes = iData(iVec);
  uint8_t* vEnd = (vBytes + i(iVec)->len) - sizeof(vectlen);
  itemnum vNumItem = readNum(vEnd, sizeof(vectlen));
  //Copy item descriptors from end of vector onto the item stack
  //  overwriting the vector item descriptor
  itemlen descsLen = vNumItem * sizeof(Item);
  memcpy(((uint8_t*)iLast() + sizeof(Item)) - descsLen, vEnd - descsLen, descsLen);
  //Adjust number of items and bytes, noting the original vector no longer exists
  numItem((numItem() - 1) + vNumItem);
  numByte((numByte() - descsLen) - sizeof(vectlen));
}


void Machine::op_Equal (itemnum firstParam, bool equality) {
  //Find equity through byte comparison, and equality through item comparison
  itemnum it = firstParam + 1, itEnd = numItem();
  for (; it < itEnd; ++it) {
    Item* a = i(firstParam);
    Item* b = i(it);
    //Equity through byte comparison
    itemlen len = a->len;
    if (len != b->len) break;
    if (memcmp(iData(firstParam), iData(it), len)) break;
    //Further equality through item comparison
    if (equality && a->type() != b->type()) break;
  }
  returnItem(firstParam, Item(0, it == itEnd ? Val_True : Val_False));
}

void Machine::op_Diff (itemnum firstParam, IType op) {
  int32_t prev = (op == Op_GT || op == Op_GTE) ? INT32_MAX : INT32_MIN;
  itemnum it = firstParam, itEnd = numItem();
  for (; it < itEnd; ++it) {
    int32_t num = readNum(iData(it), constByteLen(i(it)->type()));
    if (op == Op_GT  && num >= prev
     || op == Op_GTE && num >  prev
     || op == Op_LT  && num <= prev
     || op == Op_LTE && num <  prev)
      break;
    prev = num;
  }
  returnItem(firstParam, Item(0, it == itEnd ? Val_True : Val_False));
}

void Machine::op_Arith (itemnum firstParam, IType op) {
  if (firstParam == numItem()) {
    returnNil(firstParam);
    return;
  }
  IType type = i(firstParam)->type();
  itemlen len = constByteLen(type);
  int32_t result = readNum(iData(firstParam), len);
  for (itemnum it = firstParam + 1, itEnd = numItem(); it < itEnd; ++it) {
    int32_t num = readNum(iData(it), min(len, constByteLen(i(it)->type())));
    switch (op) {
      case Op_Add:  result += num; break;
      case Op_Sub:  result -= num; break;
      case Op_Mult: result *= num; break;
      case Op_Div:  result /= num; break;
      case Op_Mod:  result %= num; break;
    }
  }
  writeNum(iBytes(firstParam), result, len);
  returnItem(firstParam, Item(len, type));
}

void Machine::op_Str (itemnum firstParam) {
  uint8_t* result = stackItem();
  strilen len = 0;
  for (itemnum it = firstParam, itEnd = numItem(); it < itEnd; ++it) {
    Item* item = i(it);
    uint8_t* target = result + len;
    IType type = item->type();
    switch (type) {
      case Val_True: case Val_False:
        *target = type == Val_True ? 'T' : 'F';
        ++len;
        break;
      case Val_Str:
        memcpy(target, iData(it), item->len - 1);
        len += item->len - 1;
        break;
      case Val_U08:
      case Val_U16:
      case Val_I32:
        len += int2chars(target, readNum(iData(it), constByteLen(type)));
        break;
      case Val_Nil:
        const char* sNil = "nil";
        memcpy(target, sNil, 3);
        len += 3;
        break;
    }
  }
  result[len++] = 0;
  returnCollapseItem(firstParam, Item(len, Val_Str));
}

void Machine::op_Vec (itemnum firstParam) {
  //Copy item descriptors onto the end of the byte stack
  uint8_t* descs = stackItem();
  vectlen nItems = numItem() - firstParam;
  bytenum itemsLen = sizeof(Item) * nItems;
  memcpy(descs, iLast(), itemsLen);
  //Append number of items
  writeNum(descs + itemsLen, nItems, sizeof(vectlen));
  //Return umbrella item descriptor
  bytenum bytesLen = itemsBytesLen(firstParam, numItem()) + itemsLen + sizeof(vectlen);
  returnItem(firstParam, Item(bytesLen, Val_Vec));
}

void Machine::op_Nth (itemnum firstParam) {
  int32_t nth = iInt(firstParam + 1);
  //Collate vector info
  uint8_t* vBytes = iData(firstParam);
  uint8_t* vEnd = (vBytes + i(firstParam)->len) - sizeof(vectlen);
  itemnum vNumItem = readNum(vEnd, sizeof(vectlen));
  Item* vItems = &((Item*)vEnd)[-vNumItem];
  //Find item descriptor at nth
  Item* nthItem = &((Item*)vEnd)[-(nth + 1)];
  //Copy bytes into return position
  uint8_t* itemBytes = vBytes;
  for (itemnum vi = 0; vi < nth; ++vi)
    itemBytes += itemBytesLen(&vItems[vi]);
  memmove(iBytes(firstParam), itemBytes, itemBytesLen(nthItem));
  //Return nth item descriptor
  returnItem(firstParam, nthItem);
}

void Machine::op_Len (itemnum firstParam) {
  Item* item = i(firstParam);
  uint8_t* itData = iData(firstParam);
  uint32_t len = item->len;
  switch (item->type()) {
    case Val_Vec:
      len = readNum((itData + len) - sizeof(vectlen), sizeof(vectlen));
      break;
    case Val_Str: --len; break;
  }
  *(uint32_t*)iBytes(firstParam) = len;
  returnItem(firstParam, Item(sizeof(uint32_t), Val_I32));
}

void Machine::op_AppRed (itemnum firstParam, bool isReduce) {
  //Extract function or op number from first parameter
  bool isOp = i(firstParam)->type() == Var_Op;
  funcnum fCode = readNum(iData(firstParam), isOp ? sizeof(IType) : sizeof(funcnum));
  //Burst vector in situ (the last item on the stack)
  burstVec();
  //If is reduce op
  if (isReduce) {
    //Copy seed or first item onto stack - either (reduce f v) (reduce f s v)
    {
      Item* seed = i(firstParam + 1);
      memcpy(stackItem(), iBytes(firstParam + 1), itemBytesLen(seed));
      stackItem(seed);
    }
    //Reduce loop, where the stack is now: [burst v]*N [seed: either v0 or seed]
    itemnum iSeed = numItem() - 1;
    for (itemnum it = firstParam + 2; it < iSeed; ++it) {
      //Copy next item onto stack
      Item* iNext = i(it);
      memcpy(stackItem(), iBytes(it), itemBytesLen(iNext));
      stackItem(iNext);
      //Execute func or op, which returns to iSeed
      if (isOp) nativeOp((IType)fCode, iSeed);
      else      exeFunc(fCode, iSeed);
    }
  } else
  //Is apply op
  {
      if (isOp) nativeOp((IType)fCode, firstParam + 1);
      else      exeFunc(fCode, firstParam + 1);
  }
  //Collapse return
  returnCollapseLast(firstParam);
}

void Machine::op_Val (itemnum firstParam) {
  //Truncate the stack to the first item
  bytenum newNumBytes = (numByte() - itemsBytesLen(firstParam + 1, numItem()));
  numItem(firstParam + 1);
  numByte(newNumBytes);
}

void Machine::op_Do (itemnum firstParam) {
  //Collapse the stack to the last item
  returnCollapseLast(firstParam);
}

void Machine::op_MsNow (itemnum firstParam) {
  *(int32_t*)iBytes(firstParam) = msNow();
  returnItem(firstParam, Item(sizeof(int32_t), Val_I32));
}

void Machine::op_Print (itemnum firstParam) {
  op_Str(firstParam);
  debugger((const char*)iData(firstParam), false, 0);
  returnNil(firstParam);
}