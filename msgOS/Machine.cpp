#include "Machine.hpp"

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
  return (Item*)(pItems() - ((iNum+1) * sizeof(Item)));
}
uint8_t* Machine::iBytes (itemnum iNum) {
  return pBytes() + itemsBytesLen(0, iNum);
}
uint8_t* Machine::iData (itemnum iNum) {
  uint8_t* bPtr = iBytes(iNum);
  if (i(iNum)->isConst())
    return pROM() + (*(proglen*)bPtr);
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
  return pBytes() + numByte();
}
void Machine::stackItem (Item* desc) {
  numItem(numItem() + 1);
  numByte(numByte() + itemBytesLen(desc));
  memcpy(iLast(), desc, sizeof(Item));
}
void Machine::stackItem (Item desc) {
  stackItem(&desc);
}
void Machine::setStackN (itemnum newN) {
  numItem(newN);
  numByte(itemsBytesLen(0, newN));
//Erase stack onward, for improved debug readability
if (newN)
  memset(stackItem(), 0, (uint8_t*)iLast() - stackItem());
}
uint8_t* Machine::returnItem (itemnum replace) {
  return pBytes() + itemsBytesLen(0, replace);
}
void Machine::returnItem (itemnum replace, Item* desc) {
  //Replace item currently in the return position
  memcpy(i(replace), desc, sizeof(Item));
  //Update number of items and bytes
  setStackN(replace + 1);
}
void Machine::returnCollapseLast (itemnum replace) {
  itemlen lLen = itemBytesLen(iLast());
  //Move item bytes
  memmove(returnItem(replace), stackItem() - lLen, lLen);
  //Set old item in return position
  returnItem(replace, iLast());
}
void Machine::returnCollapseItem (itemnum replace, Item* desc) {
  //Move item bytes
  memmove(returnItem(replace), stackItem(), desc->len);
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



bool isTypeTruthy (IType type) {
  return type != Val_Nil && type != Val_False;
}

void skipFormOrVal (uint8_t** f) {
  //Is a value?
  if (**f > FORMS_END) {
    IType type = (IType)**f;
    *f += constByteLen(type, ++*f);
    return;
  }
  //It's a form - skip f to the end of it
  uint8_t nForm = 0;
  do {
    if (**f <= FORMS_END) ++nForm;
    else if (**f < OPS_START) {
      IType type = (IType)**f;
      *f += constByteLen(type, ++*f);
      continue;
    }
    else if (**f >= OPS_START) --nForm;
    ++*f;
  } while (nForm);
}



void Machine::heartbeat (prognum _pNum) {
  pNum = _pNum;
  exeFunc(0x0000, -1);
  //Discard heartbeat function result
  iPop();
while (true);
}

uint8_t* Machine::pFunc (funcnum fNum) {
  uint8_t* r = pROM();
  uint8_t* rEnd = pBytes();
  while (r != rEnd) {
    if (*(funcnum*)r == fNum)
      return r;
    r += sizeof(funcnum);
    r += *(proglen*)r + sizeof(proglen);
  }
  return pROM();
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

uint8_t* Machine::exeForm (uint8_t* f, itemnum firstParam) {
  IType formCode = (IType)*f;
  ++f; //Skip form code
  itemnum firstArgItem = numItem();
  bool ifWasTrue = false;
  while (true) {

    //If we're in a special form
    if (formCode != Form_Eval) {
      //Special form logic
      switch (formCode) {
        case Form_If:
          //Is if-form finished?
          if (*f == Op_If) {
            returnNil(firstArgItem);
            return ++f;
          }
          if (firstArgItem == numItem()) break; //Nothing evaluted yet
          //(if cond if-true if-false)
          //                ^
          if (ifWasTrue)
            //Was true: skip the if-false form or val (usually to the terminating if op)
            skipFormOrVal(&f);
          else
          //(if cond if-true if-false)
          //        ^
          if (firstArgItem + 1 == numItem()) {
            if (!isTypeTruthy(iLast()->type()))
              //False: skip the if-true form or val
              skipFormOrVal(&f);
            setStackN(firstArgItem); //Forget condition item
            ifWasTrue = true;
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
            setStackN(firstArgItem); //Forget condition item
          //Previous eval was true
          else {
            //Skip all forms until Op_Or
            while (*f != Op_Or)
              skipFormOrVal(&f);
            return ++f;
          }
          break;

        case Form_And:
          bool evaled = firstArgItem != numItem();
          if (evaled) {
            //Test previous eval
            bool isTruthy = isTypeTruthy(iLast()->type());
            setStackN(firstArgItem); //Forget condition item
            //If falsey
            if (!isTruthy) {
              //Skip all forms until Op_And
              while (*f != Op_And)
                skipFormOrVal(&f);
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
    if (type <= FORMS_END) { //Form?
      f = exeForm(f, firstParam);
    } else
    if (*f == Eval_Arg) { //Arg?
      itemnum iNum = firstParam + *(++f);
      Item* iArg = i(iNum);
      //Copy the referenced value to the stack top
      memcpy(stackItem(), iData(iNum), iArg->len);
      //Copy its item descriptor too, as value
      stackItem(Item(iArg->len, iArg->type()));
      f += sizeof(argnum);
    } else
    if (*f == Eval_Var) { //Variable evaluation?
      varnum vNum = *(varnum*)(++f);
      f += sizeof(varnum);
      itemnum it;
      bool found = false;
      for (it = numItem() - 1; ; --it) {
        if (i(it)->type() != Bind_Var) continue;
        if (*(varnum*)iData(it) == vNum) {
          found = true;
          break;
        }
        if (!it) break;
      }
      if (found) {
        Item* vItem = i(++it); //Pick the item after the bind
        //Copy the bytes to the stack top
        memcpy(stackItem(), iBytes(it), vItem->len);
        //Copy its item descriptor too
        stackItem(vItem);
      } else
        stackItem(Item(0, Val_Nil)); //No binding found - return nil
    } else
    if (*f < OPS_START) { //Constant?
      Item item = Item(constByteLen(type, ++f), type, true);
      *(proglen*)stackItem() = f - pROM();
      stackItem(item);
      f += constByteLen(type, f);
    } else
    if (*f == Op_Func) { //Func?
      funcnum fNum = *(funcnum*)(++f);
      exeFunc(fNum, firstArgItem);
      f += sizeof(funcnum);
      break;
    } else { //Native op?
      nativeOp((IType)*f, firstArgItem);
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
    case Op_Add:   op_Add  (firstParam); break;
    case Op_Str:   op_Str  (firstParam); break;
    case Op_Print: op_Print(firstParam); break;
    case Op_Vec:   op_Vec  (firstParam); break;
    case Op_Nth:   op_Nth  (firstParam); break;
    case Op_Val:   op_Val  (firstParam); break;
    case Op_Do:    op_Do   (firstParam); break;
    default: break;
  }
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

void Machine::op_Add (itemnum firstParam) {
  IType type = i(firstParam)->type();
  itemlen len = constByteLen(type);
  int32_t sum = 0;
  for (itemnum it = firstParam, itEnd = numItem(); it < itEnd; ++it)
    sum += readNum(iData(it), min(len, constByteLen(i(it)->type())));
  writeNum(stackItem(), sum, len);
  returnCollapseItem(firstParam, Item(len, type));
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

void Machine::op_Print (itemnum firstParam) {
  op_Str(firstParam);
  debugger((const char*)iData(firstParam), false, 0);
  returnNil(firstParam);
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
  memcpy(stackItem(), itemBytes, itemBytesLen(nthItem));
  //Return nth item descriptor
  returnCollapseItem(firstParam, nthItem);
}

void Machine::op_Val (itemnum firstParam) {
  //Truncate the stack to the first item
  setStackN(firstParam + 1);
}

void Machine::op_Do (itemnum firstParam) {
  //Collapse the stack to the last item
  returnCollapseLast(firstParam);
}
