#include "Machine.hpp"

ProgInfo progs[NUM_PROG];

Machine::Machine () {}

void Machine::romLen (proglen len) {
  pInfo->romLen = len;
  pBytes = pROM + len;
}
proglen Machine::romLen () {
  return pInfo->romLen;
}
bytenum ramOffset (prognum pNum) {
  bytenum offset = 0;
  for (prognum p = 0; p < pNum; ++p)
    offset += progs[p].ramLen;
  return offset;
}
void Machine::setPNum (prognum n) {
  pNum = n;
  pROM = mem + ramOffset(pNum);
  pFirstItem = (pROM + progs[n].ramLen) - sizeof(Item);
  pInfo = progs + pNum;
}
void Machine::numItem (itemnum n) {
  pInfo->numItem = n;
}
itemnum Machine::numItem () {
  return pInfo->numItem;
}
void Machine::numByte (bytenum n) {
  pInfo->numByte = n;
}
bytenum Machine::numByte () {
  return pInfo->numByte;
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
    return pROM + *(proglen*)bPtr;
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

void Machine::trunStack (itemnum to) {
  numByte(numByte() - itemsBytesLen(to, numItem()));
  numItem(to);
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
//Copies N items to the end of the stack
void Machine::restackCopy (itemnum from, itemnum nItem) {
  bytenum nByte = itemsBytesLen(from, from + nItem);
  //Copy bytes and descriptors
  memcpy(stackItem(), iBytes(from), nByte);
  memcpy(i(numItem()) - (nItem - 1), i(from) - (nItem - 1), nItem * sizeof(Item));
  numItem(numItem() + nItem);
  numByte(numByte() + nByte);
}
void Machine::returnNil (itemnum replace) {
  returnItem(replace, Item(0, Val_Nil));
}

void Machine::iPop () {
  trunStack(numItem() - 1);
}
void Machine::collapseItems (itemnum to, itemnum nItem) {
  itemnum from = numItem() - nItem;
  itemlen iBytesLen = itemsBytesLen(from, numItem());
  //Move item bytes and reduce number
  memcpy(iBytes(to), stackItem() - iBytesLen, iBytesLen);
  numByte(numByte() - itemsBytesLen(to, from));
  //Move item descriptors and reduce number
  memcpy(i((to + nItem) - 1), iLast(), sizeof(Item) * nItem);
  numItem(to + nItem);
}



void Machine::entry () {
  exeFunc(0x0000, 0);
}

void Machine::heartbeat (prognum _pNum) {
  setPNum(_pNum);
  exeFunc(0x0001, 0);
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
  return nullptr;
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

bool recurring = false;
void Machine::exeFunc (funcnum fNum, itemnum firstParam) {
  uint8_t* f = pFunc(fNum);
  if (f == nullptr) return;
  uint8_t* fEnd;
  f += sizeof(funcnum);
  {
    funclen fLen = *(funclen*)f;
    f += sizeof(funclen);
    fEnd = f + fLen;
  }
  uint8_t* fStart = f;
  itemnum nParam = numItem() - firstParam;
  while (f != fEnd) {
    f = exeForm(f, fEnd, firstParam, nParam);
    if (recurring) {
      nParam = numItem() - firstParam;
      recurring = false;
      f = fStart;
      continue;
    }
    if (f != fEnd)
      iPop();
  }
  //Move last item into return position
  if (numItem() > 0)
    returnCollapseLast(firstParam);
}


//Collapse args into params position
void Machine::collapseArgs (itemnum firstParam, itemnum& firstArgItem) {
  collapseItems(firstParam, numItem() - firstArgItem);
  firstArgItem = firstParam;
}
void Machine::tailCallOptim (IType type, uint8_t* f, uint8_t* funcEnd, itemnum firstParam, itemnum& firstArgItem) {
  if (f == funcEnd - constByteLen(type) - 1) //If the op is a tail call
    collapseArgs(firstParam, firstArgItem);
}

enum IfResult : uint8_t { UnEvaled = 0, WasTrue, WasFalse };

union SpecialFormData {
  IfResult ifData = UnEvaled;
};

uint8_t* Machine::exeForm (uint8_t* f, uint8_t* funcEnd, itemnum firstParam, itemnum nParam) {
  IType formCode = *(IType*)f;
  ++f; //Skip form code
  itemnum firstArgItem = numItem();
  SpecialFormData formData;
  while (!recurring) {

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
          if (formData.ifData == WasTrue) {
            //Was true: skip the if-false arg if present
            if (*f != Op_If) skipArg(&f);
            return ++f; //Skip if op
          } else
          //(if false if-true if-false)
          //                          ^
          if (formData.ifData == WasFalse) {
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
              formData.ifData = WasFalse;
            } else formData.ifData = WasTrue;
            iPop(); //Forget condition item
          }
          break;

        case Form_Or:
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
          //Did or-form end without truthy value?
          if (*f == Op_Or) {
            returnNil(firstArgItem);
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
    if (type <= FORMS_END)
      f = exeForm(f, funcEnd, firstParam, nParam);
    else
    //If a parameter
    if (type == Param_Val) {
      itemnum paramNum = *(argnum*)(++f);
      f += sizeof(argnum);
      //If parameter is outside bounds, stack nil
      if (paramNum >= nParam) {
        stackItem(Item(sizeof(Val_Nil), Val_Nil));
        continue;
      }
      paramNum += firstParam;
      Item* iParam = i(paramNum);
      //Copy the referenced value to the stack top
      memcpy(stackItem(), iData(paramNum), iParam->len);
      //Copy its item descriptor too, as value
      stackItem(Item(iParam->len, iParam->type()));
    } else
    //If a vector of the function arguments
    if (type == Val_Args) {
      restackCopy(firstParam, nParam);
      op_Vec(firstArgItem);
      ++f; //Skip const code
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
        memcpy(stackItem(), iBytes(it), itemBytesLen(vItem));
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
    //If an explicit function recursion
    if (type == Op_Recur) {
      //Treat as if tail-call and set recur flag
      collapseArgs(firstParam, firstArgItem);
      recurring = true;
    } else
    //If a program function
    if (type == Op_Func) {
      tailCallOptim(type, f, funcEnd, firstParam, firstArgItem);
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
        if (type == Var_Func) {
          tailCallOptim(type, f, funcEnd, firstParam, firstArgItem);
          exeFunc(*(funcnum*)iData(it), firstArgItem);
        } else
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
    case Op_Equal: case Op_Nequal: case Op_Equit: case Op_Nequit:
      op_Equal(firstParam, op == Op_Equal || op == Op_Nequal);
      if (op == Op_Nequal || op == Op_Nequit) negate(firstParam);
      break;
    case Op_GT: case Op_GTE: case Op_LT: case Op_LTE:
      op_Diff(firstParam, op); break;
    case Op_Add: case Op_Sub: case Op_Mult: case Op_Div: case Op_Mod:
    case Op_BNot: case Op_BAnd: case Op_BOr: case Op_BXor: case Op_LShift: case Op_RShift:
      op_Arith(firstParam, op); break;
    case Op_Str:    op_Str   (firstParam); break;
    case Op_Type:   op_Type  (firstParam); break;
    case Op_Cast:   op_Cast  (firstParam); break;
    case Op_Vec:    op_Vec   (firstParam); break;
    case Op_Nth:    op_Nth   (firstParam); break;
    case Op_Len:    op_Len   (firstParam); break;
    case Op_Sect: case Op_BSect:
      op_Sect(firstParam, op == Op_BSect); break;
    case Op_Burst:  burstItem();           break;
    case Op_Reduce: op_Reduce(firstParam); break;
    case Op_Map:    op_Map   (firstParam); break;
    case Op_For:    op_For   (firstParam); break;
    case Op_Val:    op_Val   (firstParam); break;
    case Op_Do:     op_Do    (firstParam); break;
    case Op_MsNow:  op_MsNow (firstParam); break;
    case Op_Print:  op_Print (firstParam); break;
    default: break;
  }
}


void Machine::burstItem () {
  itemnum iVec = numItem() - 1;
  Item* itVec = i(iVec);
  //If nil, destroy the item
  if (itVec->type() == Val_Nil) {
    trunStack(iVec);
    return;
  }
  //If a string, burst as characters
  if (itVec->type() == Val_Str) {
    //Ensure it is copied as value
    op_Val(iVec);
    //Remove the original string item descriptor
    trunStack(iVec);
    //Generate an Item for each character
    for (itemlen s = 0, sLen = itVec->len - 1; s < sLen; ++s)
      stackItem(Item(1, Val_Char));
    return;
  }
  uint8_t* vBytes = iBytes(iVec);
  uint8_t* vEnd = (vBytes + itVec->len) - sizeof(vectlen);
  itemnum vNumItem = readNum(vEnd, sizeof(vectlen));
  //Copy item descriptors from end of vector onto the item stack
  //  overwriting the vector item descriptor
  itemlen descsLen = vNumItem * sizeof(Item);
  memcpy(((uint8_t*)iLast() + sizeof(Item)) - descsLen, vEnd - descsLen, descsLen);
  //Adjust number of items and bytes, noting the original vector no longer exists
  numItem((numItem() - 1) + vNumItem);
  numByte((numByte() - descsLen) - sizeof(vectlen));
}

vectlen Machine::vectLen (itemnum it) {
  return readNum(iBytes(it + 1) - sizeof(vectlen), sizeof(vectlen));
}


void Machine::negate (itemnum firstParam) {
  returnItem(firstParam, Item(0, isTypeTruthy(i(firstParam)->type()) ? Val_False : Val_True));
}


void Machine::op_Equal (itemnum firstParam, bool equality) {
  //Find equity (==) through byte comparison, and equality (=) through item or int comparison
  itemnum it = firstParam + 1, itEnd = numItem();
  Item* a = i(firstParam);
  int32_t aNum = iInt(firstParam);
  bool isInt = isTypeInt(a->type());
  for (; it < itEnd; ++it) {
    Item* b = i(it);
    //When an int
    if (equality && isInt) {
      int32_t bNum = iInt(it);
      if (aNum == bNum) continue;
      break;
    }
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
  if (op == Op_BNot) result = ~result;
  for (itemnum it = firstParam + 1, itEnd = numItem(); it < itEnd; ++it) {
    int32_t num = readNum(iData(it), min(len, constByteLen(i(it)->type())));
    switch (op) {
      case Op_Add:    result +=  num; break;
      case Op_Sub:    result -=  num; break;
      case Op_Mult:   result *=  num; break;
      case Op_Div:    result /=  num; break;
      case Op_Mod:    result %=  num; break;
      case Op_BAnd:   result &=  num; break;
      case Op_BOr:    result |=  num; break;
      case Op_BXor:   result ^=  num; break;
      case Op_LShift: result <<= num; break;
      case Op_RShift: result >>= num; break;
    }
  }
  memcpy(iBytes(firstParam), &result, len);
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
      case Val_Char:
        *target = *iData(it);
        ++len;
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

void Machine::op_Type (itemnum firstParam) {
  *(IType*)iBytes(firstParam) = i(firstParam)->type();
  returnItem(firstParam, Item(sizeof(IType), fitInt(sizeof(IType))));
}

void Machine::op_Cast (itemnum firstParam) {
  IType to = (IType)readNum(iData(firstParam + 1), sizeof(IType));
  returnItem(firstParam, Item(constByteLen(to), to, i(firstParam)->isConst()));
}

void Machine::op_Vec (itemnum firstParam) {
  //Copy item descriptors onto the end of the byte stack
  uint8_t* descs = stackItem();
  vectlen nItems = numItem() - firstParam;
  bytenum itemsLen = sizeof(Item) * nItems;
  memcpy(descs, iLast(), itemsLen);
  //Append number of items
  *(vectlen*)(descs + itemsLen) = nItems;
  //Return umbrella item descriptor
  bytenum bytesLen = itemsBytesLen(firstParam, numItem()) + itemsLen + sizeof(vectlen);
  returnItem(firstParam, Item(bytesLen, Val_Vec));
}

void Machine::op_Nth (itemnum firstParam) {
  int32_t nth = iInt(firstParam + 1);
  Item* it = i(firstParam);
  //Return nil on negative nth or non-str/vec
  if (nth < 0 || (it->type() != Val_Vec && it->type() != Val_Str)) {
    returnNil(firstParam);
    return;
  }
  //Different behaviour for if a string or a vector
  if (it->type() == Val_Str) {
    if (it->len < 2 || nth > it->len - 2) {
      returnNil(firstParam);
      return;
    }
    *iBytes(firstParam) = iData(firstParam)[nth];
    returnItem(firstParam, Item(1, Val_Char));
    return;
  }
  //Collate vector info
  uint8_t* vBytes = iData(firstParam);
  uint8_t* vEnd = (vBytes + it->len) - sizeof(vectlen);
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
  itemlen len = item->len;
  switch (item->type()) {
    case Val_Vec: len = vectLen(firstParam); break;
    case Val_Str: --len; break;
  }
  *(itemlen*)iBytes(firstParam) = len;
  returnItem(firstParam, Item(sizeof(itemlen), fitInt(sizeof(itemlen))));
}

void Machine::op_Sect (itemnum firstParam, bool isBurst) {
  IType type = i(firstParam)->type();
  bool isStr = type == Val_Str;
  //Return nil if not a vector or string
  if (type != Val_Vec && !isStr) {
    returnNil(firstParam);
    return;
  }
  //Get vector or string length
  vectlen len = isStr ? i(firstParam)->len - 1 : vectLen(firstParam);
  //Retain the skip and take, or use defaults
  vectlen skip = 1,
          take = len - skip;
  {
    argnum nArg = numItem() - firstParam;
    if (nArg > 1) skip = iInt(firstParam + 1);
    if (nArg > 2) take = iInt(firstParam + 2);
  }
  //Bound skip and take to length
  if (skip >= len) {
    //Return empty vector or string if skip is beyond length
    if (isStr) {
      *(char*)stackItem() = 0;
      stackItem(Item(1, Val_Str));
    } else {
      *(vectlen*)stackItem() = 0;
      stackItem(Item(sizeof(vectlen), Val_Vec));
    }
    returnCollapseLast(firstParam);
    return;
  }
  if (skip + take > len)
    take = (skip + take) - len;
  if (isStr) {
    //Copy subsection of memory to start of the string, add terminator
    memcpy(iBytes(firstParam), iData(firstParam) + skip, take);
    iBytes(firstParam)[take + 1] = 0;
    //Either return burst characters (b-sect) or a string (sect)
    returnItem(firstParam, Item(take + 1, Val_Str));
    if (isBurst) burstItem();
    return;
  }
  //Vector becomes only parameter and is burst
  trunStack(firstParam + 1);
  burstItem();
  //Truncate to skip+take
  trunStack(firstParam + skip + take);
  //Either return burst items (b-sect) or a vector (sect)
  if (isBurst) {
    collapseItems(firstParam, take);
  } else {
    op_Vec(firstParam + skip);
    returnCollapseLast(firstParam);
  }
}

void Machine::op_Reduce (itemnum firstParam) {
  //Extract function or op number from first parameter
  bool isOp = i(firstParam)->type() == Var_Op;
  funcnum fCode = readNum(iData(firstParam), isOp ? sizeof(IType) : sizeof(funcnum));
  //Burst item in situ (the last item on the stack)
  burstItem();
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
  //Collapse return
  returnCollapseLast(firstParam);
}

void Machine::op_Map (itemnum firstParam) {
  //Extract function or op number from first parameter
  bool isOp = i(firstParam)->type() == Var_Op;
  funcnum fCode = readNum(iData(firstParam), isOp ? sizeof(IType) : sizeof(funcnum));
  //Find shortest vector
  itemnum iFirstVec = firstParam + 1;
  itemnum nVec = numItem() - iFirstVec;
  vectlen shortest = -1;
  for (itemnum it = iFirstVec, itEnd = iFirstVec + nVec; it < itEnd; ++it) {
    vectlen l = vectLen(it);
    if (l < shortest) shortest = l;
  }
  //Map loop
  itemnum iMapped = firstParam + 1 + nVec;
  uint8_t* pFirstVec = iBytes(iFirstVec);
  for (itemnum it = 0; it < shortest; ++it) {
    bytenum descOffset = sizeof(Item) * it;
    for (itemnum vi = iFirstVec, viEnd = iFirstVec + nVec; vi < viEnd; ++vi) {
      //Fast-fetch descriptor and bytes
      //  by using a byte offset in the cannibalised first Item
      uint8_t* vFirstDesc = pFirstVec + ((itemsBytesLen(iFirstVec, vi + 1) - sizeof(vectlen)) - sizeof(Item));
      itemlen viBytesOffset = it ? *(itemlen*)vFirstDesc
                                 : itemsBytesLen(iFirstVec, vi);
      Item iDesc = *(Item*)(vFirstDesc - descOffset);
      if (it) *(itemlen*)vFirstDesc += itemBytesLen(&iDesc);
      else    *(itemlen*)vFirstDesc  = viBytesOffset + itemBytesLen(&iDesc);
      //Copy onto stack
      memcpy(stackItem(), pFirstVec + viBytesOffset, itemBytesLen(&iDesc));
      stackItem(iDesc);
    }
    if (isOp) nativeOp((IType)fCode, iMapped + it);
    else      exeFunc(fCode, iMapped + it);
  }
  //Vectorise and collapse return
  op_Vec(iMapped);
  returnCollapseLast(firstParam);
}

void Machine::op_For (itemnum firstParam) {
  //Extract function or op number from first parameter
  bool isOp = i(firstParam)->type() == Var_Op;
  funcnum fCode = readNum(iData(firstParam), isOp ? sizeof(IType) : sizeof(funcnum));
  //Output N byte lengths and N byte counters onto stack as blob item
  itemnum iFirstVec = firstParam + 1;
  argnum nVec = numItem() - iFirstVec;
  itemlen* lens    = (vectlen*)stackItem(); //Holds param vector lengths
  itemlen* counts  = lens + nVec;           //Holds loop vector indexes
  itemlen* offsets = counts + nVec;         //Holds loop vector byte offsets
  itemlen* stagedOffsets = offsets + nVec;  //Holds offset for next vector byte offset increment
  {
    vectlen nLen = 0;
    for (itemnum v = iFirstVec, vEnd = iFirstVec + nVec; v < vEnd; ++v)
      lens[nLen++] = vectLen(v);
  }
  memset(counts,  0, sizeof(itemlen) * nVec * 3);
  stackItem(Item(sizeof(itemlen) * nVec * 4, Val_Blob));
  //For loop
  uint8_t* pFirstVec = iBytes(iFirstVec);
  bool forLoop = true;
  while (forLoop) {
    //Output one of each item, indexed by each counter
    itemnum exeAt = numItem();
    for (argnum v = 0, vLast = nVec - 1; v < nVec; ++v) {
      bytenum descOffset = sizeof(Item) * counts[v];
      //Fast-fetch descriptor and bytes
      //  by using a byte offset in the byte counter
      uint8_t* vStart = pFirstVec + itemsBytesLen(iFirstVec, iFirstVec + v);
      uint8_t* vFirstDesc = vStart + i(iFirstVec + v)->len - sizeof(vectlen) - sizeof(Item);
      uint8_t* viBytes = vStart + offsets[v];
      Item desc = *(Item*)(vFirstDesc - descOffset);
      itemlen iLen = itemBytesLen(&desc);
      stagedOffsets[v] = iLen;
      //Increment end counter
      if (v == vLast) {
        ++counts[v];
        offsets[v] += iLen;
        //Check & reset counters
        argnum v = 0;
        for (; v < nVec; ++v) {
          argnum reverseV = (nVec - 1) - v;
          vectlen* c = &counts[reverseV];
          if (*c == lens[reverseV]) {
            *c = 0;
            offsets[reverseV] = 0;
            if (reverseV) {
              argnum nextV = reverseV - 1;
              ++counts[nextV];
              offsets[nextV] += stagedOffsets[nextV];
            }
          } else break;
        }
        //If counters all fully incremented, for loop is complete
        if (v == nVec)
          forLoop = false;
      }
      //Copy onto stack
      memcpy(stackItem(), viBytes, iLen);
      stackItem(desc);
    }
    //Reduce extracted values
    if (isOp) nativeOp((IType)fCode, exeAt);
    else      exeFunc(fCode, exeAt);
  }
  //Vectorise and collapse return
  op_Vec(firstParam + 1 + nVec + 1);
  returnCollapseLast(firstParam);
}

void Machine::op_Val (itemnum firstParam) {
  //Truncate the stack to the first item
  trunStack(firstParam + 1);
  //If it's const, copy as value
  Item* it = i(firstParam);
  if (it->isConst()) {
    memcpy(iBytes(firstParam), iData(firstParam), it->len);
    returnItem(firstParam, Item(it->len, it->type(), false));
  }
}

void Machine::op_Do (itemnum firstParam) {
  //Collapse the stack to the last item
  returnCollapseLast(firstParam);
}

void Machine::op_MsNow (itemnum firstParam) {
  *(int32_t*)iBytes(firstParam) = harness->msNow();
  returnItem(firstParam, Item(sizeof(int32_t), Val_I32));
}

void Machine::op_Print (itemnum firstParam) {
  op_Str(firstParam);
  harness->print((const char*)iData(firstParam));
  returnNil(firstParam);
}