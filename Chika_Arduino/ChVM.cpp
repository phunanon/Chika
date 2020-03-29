#include "ChVM.hpp"

prognum   pNum;       //
uint8_t*  pBytes;     //
uint8_t*  pFirstItem; //
ProgInfo* pInfo;      //
uint8_t* pROM;        // Current program data
funcnum prevFNum = -1;       //
uint8_t* prevFPtr = nullptr; // Used by ChVM::exeFunc

ChVM::ChVM (ChVM_Harness* _harness) {
  harness = _harness;
  broker = Broker();
}

bytenum ChVM::memOffset (prognum pNum) {
  bytenum offset = 0;
  for (prognum p = 0; p < pNum; ++p)
    offset += progs[p].memLen;
  return offset;
}
void ChVM::switchToProg (prognum n, proglen romLen, bytenum memLen) {
  if (romLen)
    progs[n].romLen = romLen;
  if (memLen)
    progs[n].memLen = memLen <= MAX_PROG_RAM ? memLen : MAX_PROG_RAM;
  pNum = n;
  pROM = mem + memOffset(n);
  pBytes = pROM + progs[n].romLen;
  pFirstItem = (pROM + progs[n].memLen) - sizeof(Item);
  pInfo = &progs[n];
  prevFNum = -1;      //
  prevFPtr = nullptr; // Used by ChVM::exeFunc
}
uint8_t* ChVM::getPROM () {
  return pROM;
}
void ChVM::numItem (itemnum n) {
  pInfo->numItem = n;
}
itemnum ChVM::numItem () {
  return pInfo->numItem;
}
void ChVM::numByte (bytenum n) {
  pInfo->numByte = n;
}
bytenum ChVM::numByte () {
  return pInfo->numByte;
}

//`to` is exclusive
itemlen ChVM::itemsBytesLen (itemnum from, itemnum to) {
  bytenum n = 0;
  for (itemnum it = from; it < to; ++it)
    n += i(it)->len;
  return n;
}
Item* ChVM::i (itemnum iNum) {
  return (Item*)(pFirstItem - (iNum * sizeof(Item)));
}
uint8_t* ChVM::iBytes (itemnum iNum) {
  return (pBytes + numByte()) - itemsBytesLen(iNum, numItem());
}
Item* ChVM::iLast () {
  itemnum nItem = numItem();
  if (!nItem) return nullptr;
  return i(nItem - 1);
}

int32_t ChVM::iInt (itemnum iNum) {
  return readNum(iBytes(iNum), constByteLen(i(iNum)->type));
}
bool ChVM::iBool (itemnum iNum) {
  return isTypeTruthy(i(iNum)->type);
}
bool ChVM::iCBool (itemnum iNum) {
  IType type = i(iNum)->type;
  return type != Val_False && (type == Val_True || iInt(iNum));
}
const char* ChVM::iStr (itemnum iNum) {
  return (const char*)iBytes(iNum);
}

void ChVM::trunStack (itemnum to) {
  numByte(numByte() - itemsBytesLen(to, numItem()));
  numItem(to);
}
uint8_t* ChVM::stackItem () {
  return pBytes + numByte();
}
void ChVM::stackItem (Item* desc) {
  numItem(numItem() + 1);
  numByte(numByte() + desc->len);
  memcpy(iLast(), desc, sizeof(Item));
}
void ChVM::stackItem (Item desc) {
  stackItem(&desc);
}
void ChVM::returnItem (itemnum replace, Item* desc) {
  bytenum newNumBytes = (numByte() - itemsBytesLen(replace, numItem())) + desc->len;
  //Replace item currently in the return position
  memcpy(i(replace), desc, sizeof(Item));
  //Update number of items and bytes
  numItem(replace + 1);
  numByte(newNumBytes);
}
void ChVM::returnCollapseLast (itemnum replace) {
  itemlen lLen = iLast()->len;
  //Move item bytes
  memcpy(iBytes(replace), stackItem() - lLen, lLen);
  //Set old item in return position
  returnItem(replace, iLast());
}
void ChVM::returnCollapseItem (itemnum replace, Item* desc) {
  //Move item bytes
  memcpy(iBytes(replace), stackItem(), desc->len);
  //Set new item in return position
  returnItem(replace, desc);
}
void ChVM::returnItem (itemnum replace, Item desc) {
  returnItem(replace, &desc);
}
void ChVM::returnCollapseItem (itemnum replace, Item desc) {
  returnCollapseItem(replace, &desc);
}
void ChVM::returnItemFrom (itemnum to, itemnum from) {
  Item* iFrom = i(from);
  //Copy bytes
  memcpy(iBytes(to), iBytes(from), iFrom->len);
  //Copy descriptor
  returnItem(to, iFrom);
}
//Copies N items to the end of the stack
void ChVM::restackCopy (itemnum from, itemnum nItem = 1) {
  bytenum nByte = itemsBytesLen(from, from + nItem);
  //Copy bytes and descriptors
  memcpy(stackItem(), iBytes(from), nByte);
  memcpy(i(numItem()) - (nItem - 1), i(from) - (nItem - 1), nItem * sizeof(Item));
  numItem(numItem() + nItem);
  numByte(numByte() + nByte);
}
void ChVM::returnNil (itemnum replace) {
  returnItem(replace, Item(0, Val_Nil));
}
void ChVM::returnBool (itemnum replace, bool b) {
  returnItem(replace, Item(b ? Val_True : Val_False));
}
void ChVM::stackNil () {
  stackItem(Item(0, Val_Nil));
}

void ChVM::iPop () {
  trunStack(numItem() - 1);
}
//Copies last `nItem` items to `to`
void ChVM::collapseItems (itemnum to, itemnum nItem) {
  itemnum from = numItem() - nItem;
  itemlen iBytesLen = itemsBytesLen(from, numItem());
  //Move item bytes and reduce number
  memcpy(iBytes(to), stackItem() - iBytesLen, iBytesLen);
  numByte(numByte() - itemsBytesLen(to, from));
  //Move item descriptors and reduce number
  memmove(i((to + nItem) - 1), iLast(), sizeof(Item) * nItem);
  numItem(to + nItem);
}

void ChVM::purgeProg (prognum n) {
  --numProg;
  //Move all programs right of this one into its position
  //And move program infos left in the process too
  if (pNum == numProg) return;
  bytenum len = 0;
  bytenum replaceLen = pInfo->memLen;
  for (prognum p = pNum + 1; p < numProg + 1; ++p) {
    len += progs[p].memLen;
    if (p - pNum)
      progs[p - 1] = progs[p];
  }
  memcpy(pROM, pROM + replaceLen, len);
  //Remove its subscriptions,
  //  and shift any message callbacks left too
  broker.unsubscribe(pNum);
  broker.shiftCallbacks(pNum);
}


void ChVM::entry () {
  exeFunc(0x0000, 0);
}

bool ChVM::heartbeat (prognum pNum) {
  switchToProg(pNum);
  if (pInfo->isHalting) {
    purgeProg(pNum);
    return false;
  }
  if (harness->msNow() > pInfo->sleepUntil)
    return exeFunc(0x0001, 0);
  return true;
}

bool ChVM::heartbeat () {
  bool allDead = true;
  for (uint8_t p = 0; p < numProg; ++p)
    if (heartbeat(p))
      allDead = false;
  return !allDead;
}

uint8_t* ChVM::pFunc (funcnum fNum) {
  uint8_t* r = pROM;
  uint8_t* rEnd = pBytes;
  while (r != rEnd) {
    if (readUNum(r, sizeof(funcnum)) == fNum)
      return r;
    r += sizeof(funcnum);
    r += readUNum(r, sizeof(proglen)) + sizeof(proglen);
  }
  return nullptr;
}

bool ChVM::findBind (itemnum& it, bindnum bNum, bool skipOne) {
  bool found = false;
  if (numItem() > 1) {
    it = numItem() - 1;
    for (; ; --it) {
      if (it == (itemnum)-1) break;
      if (i(it)->type != Bind_Mark) continue;
      //Test if this bind is the correct number
      if (readUNum(iBytes(it), sizeof(bindnum)) == bNum) {
        if (skipOne) {
          skipOne = false;
          continue;
        }
        found = true;
        break;
      }
    }
  }
  //The item after the bind is the variable
  return found ? ++it : false;
}

enum FuncState { FuncContinue, FuncRecur, FuncReturn, Halted };


//These globals are set and restored per function call by ChVM::exeFunc,
//  and used extensively by ChVM::exeForm, providing per-function context
uint8_t* f;
uint8_t* funcEnd;
itemnum p0;
itemnum nArg;
FuncState funcState;

bool ChVM::exeFunc (funcnum fNum, itemnum firstParam) {
  //Cache previous function attribute
  uint8_t* prev_f = f;

  if (prevFNum == fNum)
    f = prevFPtr;
  else {
    f = pFunc(fNum);
    prevFNum = fNum;
    prevFPtr = f;
  }

  if (f == nullptr) {
    f = prev_f;
    return false;
  }
  uint8_t* fEnd;
  f += sizeof(funcnum);
  {
    funclen fLen = readUNum(f, sizeof(funclen));
    if (!fLen) {
      f = prev_f;
      return false;
    }
    f += sizeof(funclen);
    fEnd = f + fLen;
  }

  //Cache previous function attributes
  uint8_t* prev_funcEnd = funcEnd;
  itemnum prev_p0 = p0;
  itemnum prev_nArg = nArg;

  p0 = firstParam;
  uint8_t* fStart = f;
  nArg = numItem() - p0;
  funcState = FuncContinue;
  while (f != fEnd) {
    exeForm();
    if (funcState == FuncRecur) {
      nArg = numItem() - p0;
      funcState = FuncContinue;
      f = fStart;
      continue;
    }
    if (funcState == FuncReturn) {
      funcState = FuncContinue;
      break;
    }
    if (funcState == Halted)
      break;
    if (f != fEnd)
      iPop();
  }
  //Move last item into return position
  if (numItem() > 1)
    returnCollapseLast(p0);

  //Restore previous function attributes
  f = prev_f;
  funcEnd = prev_funcEnd;
  p0 = prev_p0;
  nArg = prev_nArg;

  return true;
}


void ChVM::msgInvoker (prognum pN, funcnum fN, const char* topic, Item* iPayload, uint8_t* bPayload, bool provideT) {
  switchToProg(pN);
  //In the case of no program state before a message comes in, create a nil
  if (!numItem())
    returnNil(0);
  //In the case of a circular publishing, ensure a copy of the state is the next item
  else if (numItem() > 1)
    restackCopy(0);
  //Copy the topic (if provideT) & payload as items 1 & 2 (param 1 & 2),
  //  as the state is item 0,
  //  then call the function
  if (provideT) {
    strilen sLen = strlen(topic) + 1;
    memcpy(stackItem(), topic, sLen);
    stackItem(Item(sLen, Val_Str));
  }
  memcpy(stackItem(), bPayload, iPayload->len);
  stackItem(iPayload);
  exeFunc(fN, numItem() - (provideT ? 3 : 2));
}


//Collapse args into params position
void ChVM::collapseArgs (itemnum& firstArgItem) {
  collapseItems(p0, numItem() - firstArgItem);
  firstArgItem = p0;
}
void ChVM::tailCallOptim (IType type, itemnum& firstArgItem) {
  if (f == funcEnd - constByteLen(type) - 1) //If the op is a tail call
    collapseArgs(firstArgItem);
}

enum SpecialFormData { UnEvaled = 0, WasTrue, WasFalse };

IType nextEval; //Used within ChVM::exeForm()
void ChVM::exeForm () {
  IType formCode = *(IType*)f;
  ++f; //Skip form code
  itemnum firstArgItem = numItem();
  SpecialFormData formData = UnEvaled;
  while (funcState == FuncContinue) {

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
          if (formData == WasTrue) {
            //Was true: skip the if-false arg if present
            if (*f != Op_If) skipArg(&f);
            ++f; //Skip op
            return;
          } else
          //(if false if-true if-false)
          //                          ^
          if (formData == WasFalse) {
            ++f; //Skip op
            return;
          } else
          //(if cond if-true[ if-false])
          //        ^
          if (firstArgItem + 1 == numItem()) {
            if (!isTypeTruthy(iLast()->type)) {
              //False: skip the if-true arg
              skipArg(&f);
              //If there's no if-false, return nil
              if (*f == Op_If) {
                returnNil(firstArgItem);
                ++f; //Skip op
                return;
              }
              formData = WasFalse;
            } else formData = WasTrue;
            iPop(); //Forget condition item
          }
          break;

        case Form_Or:
          if (firstArgItem == numItem()) break; //Nothing evaluted yet
          //Exhaust current stack of arguments - in the case of (or (burst [1 2 3]) 1)
          do {
            //If falsey, shift the stack left by 1 to forget this condition item
            if (!iBool(firstArgItem))
              collapseItems(firstArgItem, (numItem() - firstArgItem) - 1);
            //Previous item was true
            else {
              //Truncate the stack to this condition
              trunStack(firstArgItem + 1);
              //Skip all args until Op_Or
              while (*f != Op_Or)
                skipArg(&f);
              ++f; //Skip op
              return;
            }
          } while (numItem() != firstArgItem);
          //Did or-form end without truthy value?
          if (*f == Op_Or) {
            returnNil(firstArgItem);
            ++f; //Skip op
            return;
          }
          break;

        case Form_And: {
          bool evaled = firstArgItem != numItem();
          if (evaled) {
            //Exhaust current stack of arguments - in the case of (and (burst [1 2 3]) 1)
            bool isTruthy = true;
            do {
              isTruthy = iBool(firstArgItem);
              //If falsey, shift the stack left by 1 to forget this condition item
              collapseItems(firstArgItem, (numItem() - firstArgItem) - 1);
            } while (isTruthy && numItem() != firstArgItem);
            //If falsey
            if (!isTruthy) {
              //Skip all args until Op_And
              while (*f != Op_And)
                skipArg(&f);
              returnItem(firstArgItem, Item(0, Val_False));
              ++f; //Skip op
              return;
            }
          }
          //Did and-form end without a falsey value?
          if (*f == Op_And) {
            returnBool(firstArgItem, evaled);
            ++f; //Skip op
            return;
          }
        } break;
        
        case Form_Case:
          //(case val ...)
          //     ^
          if (firstArgItem == numItem()) break; //Nothing evaluted yet
          //When the previous case matched
          if (formData == WasTrue) {
            //Return evaluated case by skipping all args until Op_Case
            while (*f != Op_Case)
              skipArg(&f);
            returnCollapseLast(firstArgItem);
            ++f; //Skip op
            return;
          }
          //(case val ...)
          //         \>>>
          if (firstArgItem + 1 < numItem()) {
            //If there's no matching cases return the default case
            if (*f == Op_Case) {
              returnCollapseLast(firstArgItem);
              ++f; //Skip op
              return;
            }
            //Copy the match value to the top of the stack and compare
            restackCopy(firstArgItem);
            op_Equal(numItem() - 2, Op_Equal);
            //If comparison was successful, flag as evaluated
            //  else skip value to next case
            if (isTypeTruthy(iLast()->type))
              formData = WasTrue;
            else
              skipArg(&f);
            //Forget case comparison
            iPop();
          }
          break;
      }
    }

    //Evaluate next
    nextEval = (IType)*f;
    //If a form
    if (nextEval <= FORMS_END)
      exeForm();
    else
    //If a parameter
    if (nextEval == Param_Val) {
      itemnum paramNum = readUNum(++f, sizeof(argnum));
      f += sizeof(argnum);
      //If parameter is outside bounds, stack nil
      if (paramNum >= nArg) {
        stackNil();
        continue;
      }
      paramNum += p0;
      Item* iParam = i(paramNum);
      //memcpy the data onto the stack
      memcpy(stackItem(), iBytes(paramNum), iParam->len);
      stackItem(Item(iParam->len, iParam->type));
    } else
    //If a vector of the function arguments
    if (nextEval == Val_Args) {
      restackCopy(p0, nArg);
      op_Vec(firstArgItem);
      ++f; //Skip const code
    } else
    //If a binding reference
    if (nextEval == Bind_Val || nextEval == XBind_Val) {
      bindnum bNum = readUNum(++f, sizeof(bindnum));
      f += sizeof(bindnum);
      itemnum it;
      //If the binding was found
      if (findBind(it, bNum, nextEval == XBind_Val)) {
        Item* bItem = i(it);
        //memcpy the data onto the stack
        memcpy(stackItem(), iBytes(it), bItem->len);
        stackItem(bItem);
      } else
      //No binding found - return nil
        stackNil();
    } else
    //If a constant
    if (nextEval < OPS_START) {
      Item item = Item(constByteLen(nextEval, ++f), nextEval);
      memcpy(stackItem(), f, item.len);
      stackItem(item);
      f += constByteLen(nextEval, f);
    } else
    //If an explicit function recursion
    if (nextEval == Op_Recur) {
      //Treat as if tail-call and set recur flag
      collapseArgs(firstArgItem);
      funcState = FuncRecur;
    } else
    //If an explicit function return
    if (nextEval == Op_Return) {
      if (firstArgItem == numItem())
        returnNil(firstArgItem);
      else returnCollapseLast(firstArgItem);
      f = funcEnd;
      funcState = FuncReturn;
    } else
    //If a program function
    if (nextEval == Op_Func) {
      tailCallOptim(nextEval, firstArgItem);
      funcnum fNum = readUNum(++f, sizeof(funcnum));
      exeFunc(fNum, firstArgItem);
      f += sizeof(funcnum);
      break;
    } else
    //If a native op or function through a binding or parameter
    if (nextEval == Op_Bind || nextEval == Op_Param) {
      itemnum it;
      bool found = true;

      if (nextEval == Op_Bind) {
        found = findBind(it, readUNum(++f, sizeof(bindnum)), false);
        f += sizeof(bindnum);
      } else {
        it = p0 + readUNum(++f, sizeof(argnum));
        f += sizeof(argnum);
      }
      
      if (!found)
      //Variable wasn't found
        returnNil(firstArgItem);
      else {
        IType type = i(it)->type;
        //If a native op
        if (type == Var_Op)
          nativeOp(*(IType*)iBytes(it), firstArgItem);
        else
        //If a program function
        if (type == Var_Func) {
          tailCallOptim(type, firstArgItem);
          exeFunc(readUNum(iBytes(it), sizeof(funcnum)), firstArgItem);
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
}

void ChVM::nativeOp (IType op, itemnum p0) {
  switch (op) {
    case Op_Case:   returnNil(p0); break; //No default case supplied
    case Op_Not:    op_Not   (p0); break;
    case Op_Equal: case Op_Nequal: case Op_Equit: case Op_Nequit:
      op_Equal(p0, op == Op_Equal || op == Op_Nequal);
      if (op == Op_Nequal || op == Op_Nequit) op_Not(p0);
      break;
    case Op_GT: case Op_GTE: case Op_LT: case Op_LTE:
      op_Diff(p0, op); break;
    case Op_Add: case Op_Sub: case Op_Mult: case Op_Div: case Op_Mod: case Op_Pow:
    case Op_BNot: case Op_BAnd: case Op_BOr: case Op_BXor: case Op_LShift: case Op_RShift:
      op_Arith(p0, op); break;
    case Op_PinMod: case Op_DigIn: case Op_AnaIn: case Op_DigOut: case Op_AnaOut:
      op_GPIO(p0, op); break;
    case Op_Read:   op_Read  (p0); break;
    case Op_Write:  op_Write (p0); break;
    case Op_Append: op_Append(p0); break;
    case Op_Delete: op_Delete(p0); break;
    case Op_Str:    op_Str   (p0); break;
    case Op_Type:   op_Type  (p0); break;
    case Op_Cast:   op_Cast  (p0); break;
    case Op_Vec:    op_Vec   (p0); break;
    case Op_Nth:    op_Nth   (p0); break;
    case Op_Len:    op_Len   (p0); break;
    case Op_Sect: case Op_BSect:
      op_Sect(p0, op == Op_BSect); break;
    case Op_Burst:  burstItem();           break;
    case Op_Reduce: op_Reduce(p0); break;
    case Op_Map:    op_Map   (p0); break;
    case Op_For:    op_For   (p0); break;
    case Op_Loop:   op_Loop  (p0); break;
    case Op_Binds:  op_Binds (p0); break;
    case Op_Val:    op_Val   (p0); break;
    case Op_Do:     op_Do    (p0); break;
    case Op_MPub:   op_Pub   (p0); break;
    case Op_MSub:   op_Sub   (p0); break;
    case Op_MUnsub: op_Unsub (p0); break;
    case Op_MsNow:  op_MsNow (p0); break;
    case Op_Sleep:  op_Sleep (p0); break;
    case Op_Print:  op_Print (p0); break;
    case Op_Debug:  op_Debug (p0); break;
    case Op_Load:   op_Load  (p0); break;
    case Op_Halt:   op_Halt(); break;
    default: break;
  }
}


void ChVM::burstItem () {
  itemnum iVec = numItem() - 1;
  Item* itVec = i(iVec);
  //If a string, burst as characters
  if (itVec->type == Val_Str) {
    //Remove the original string item descriptor
    trunStack(iVec);
    //Generate an Item for each character
    for (itemlen s = 0, sLen = itVec->len - 1; s < sLen; ++s)
      stackItem(Item(1, Val_Char));
    return;
  }
  //If not a string or vector, destroy the item
  if (itVec->type != Val_Vec) {
    trunStack(iVec);
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

vectlen ChVM::vectLen (itemnum it) {
  return readNum((iBytes(it) + i(it)->len) - sizeof(vectlen), sizeof(vectlen));
}


void ChVM::op_Not (itemnum p0) {
  returnBool(p0, !iBool(p0));
}

void ChVM::op_Equal (itemnum p0, bool equality) {
  //Find equity (==) through byte comparison, and equality (=) through item or int comparison
  itemnum it = p0 + 1, itEnd = numItem();
  Item* a = i(p0);
  int32_t aNum = iInt(p0);
  bool isInt = isTypeInt(a->type);
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
    if (memcmp(iBytes(p0), iBytes(it), len)) break;
    //Further equality through item comparison
    if (equality && a->type != b->type) break;
  }
  returnBool(p0, it == itEnd);
}

void ChVM::op_Diff (itemnum p0, IType op) {
  int32_t prev = (op == Op_GT || op == Op_GTE) ? INT32_MAX : INT32_MIN;
  itemnum it = p0, itEnd = numItem();
  for (; it < itEnd; ++it) {
    int32_t num = iInt(it);
    if (op == Op_GT  && num >= prev
     || op == Op_GTE && num >  prev
     || op == Op_LT  && num <= prev
     || op == Op_LTE && num <  prev)
      break;
    prev = num;
  }
  returnBool(p0, it == itEnd);
}

void ChVM::op_Arith (itemnum p0, IType op) {
  if (p0 == numItem()) {
    returnNil(p0);
    return;
  }
  IType type = i(p0)->type;
  itemlen len = constByteLen(type);
  int32_t result = readNum(iBytes(p0), len);
  if (op == Op_BNot) result = ~result;
  for (itemnum it = p0 + 1, itEnd = numItem(); it < itEnd; ++it) {
    int32_t num = readNum(iBytes(it), min(len, constByteLen(i(it)->type)));
    switch (op) {
      case Op_Add:    result +=  num; break;
      case Op_Sub:    result -=  num; break;
      case Op_Mult:   result *=  num; break;
      case Op_Div:    result /=  num; break;
      case Op_Mod:    result %=  num; break;
      case Op_Pow:
        result = _pow(result, num);   break;
      case Op_BAnd:   result &=  num; break;
      case Op_BOr:    result |=  num; break;
      case Op_BXor:   result ^=  num; break;
      case Op_LShift: result <<= num; break;
      case Op_RShift: result >>= num; break;
    }
  }
  memcpy(iBytes(p0), &result, len);
  returnItem(p0, Item(len, type));
}

void ChVM::op_GPIO (itemnum p0, IType op) {
  uint8_t pin = iInt(p0);
  uint16_t input = 0;
  switch (op) {
    case Op_PinMod: harness->pinMod(pin, iCBool(p0 + 1)); break;
    case Op_DigIn:  input = harness->digIn(pin); break;
    case Op_AnaIn:  input = harness->anaIn(pin); break;
    case Op_DigOut:
      for (itemnum p = p0; p < numItem(); p += 2)
        harness->digOut(iInt(p), iCBool(p + 1));
      break;
    case Op_AnaOut:
      for (itemnum p = p0; p < numItem(); p += 2)
        harness->anaOut(iInt(p), iInt(p + 1));
      break;
  }
  if (op == Op_DigIn) {
    returnBool(p0, input);
  } else
  if (op == Op_AnaIn) {
    writeUNum(iBytes(p0), input, sizeof(input));
    returnItem(p0, Item(sizeof(input), fitInt(sizeof(input))));
  } else {
    returnNil(p0);
  }
}

void ChVM::op_Read (itemnum p0) {
  //Collect arguments
  uint32_t offset = 0, count = 0;
  argnum nArg = numItem() - p0;
  if (nArg > 1) offset = iInt(p0 + 1);
  if (nArg > 2) count  = iInt(p0 + 2);
  //Read file into first parameter memory
  //TODO: crash if above RAM limit
  int32_t rLen = harness->fileRead(iStr(p0), iBytes(p0), offset, count);
  //If the file could not be opened, or read was out-of-bounds, return nil
  if (!rLen) {
    returnNil(p0);
    return;
  }
  //Return blob descriptor
  returnItem(p0, Item(rLen, Val_Blob));
}

void ChVM::op_Write (itemnum p0) {
  //Collect arguments
  uint32_t offset = 0, count = i(p0 + 1)->len;
  argnum nArg = numItem() - p0;
  if (nArg > 1) offset = iInt(p0 + 2);
  //Ensure item is etiher a blob or converted to string
  IType type = i(p0 + 1)->type;
  if (type != Val_Blob) {
    if (type != Val_Str) op_Str(p0 + 1);
    returnItem(p0 + 1, Item(i(p0 + 1)->len - 1, Val_Blob));
  }
  bool success = harness->fileWrite(iStr(p0), iBytes(p0 + 1), offset, i(p0 + 1)->len);
  returnBool(p0, success);
}

void ChVM::op_Append (itemnum p0) {
  //Ensure item is etiher a blob or converted to string
  IType type = i(p0 + 1)->type;
  if (type != Val_Blob) {
    op_Str(p0 + 1);
    returnItem(p0 + 1, Item(i(p0 + 1)->len - 1, Val_Blob));
  }
  bool success = harness->fileAppend(iStr(p0), iBytes(p0 + 1), i(p0 + 1)->len);
  returnBool(p0, success);
}

void ChVM::op_Delete (itemnum p0) {
  bool success = harness->fileDelete(iStr(p0));
  returnBool(p0, success);
}

void ChVM::op_Str (itemnum p0) {
  uint8_t* result = stackItem();
  strilen len = 0;
  for (itemnum it = p0, itEnd = numItem(); it < itEnd; ++it) {
    Item* item = i(it);
    uint8_t* target = result + len;
    IType type = item->type;
    switch (type) {
      case Val_True: case Val_False: case Val_Nil:
        *target = type == Val_True ? 'T' : (type == Val_False ? 'F' : 'N');
        ++len;
        break;
      case Val_Str:
        memcpy(target, iBytes(it), item->len - 1);
        len += item->len - 1;
        break;
      case Val_Vec: {
        target[0] = '[';
        uint8_t nLen = int2chars(target + 1, vectLen(it));
        target[nLen + 1] = ']';
        len += nLen + 2;
        break; }
      case Val_Blob:
        memcpy(target, iBytes(it), item->len);
        len += item->len;
        break;
      case Val_U08:
      case Val_U16:
      case Val_I32:
        len += int2chars(target, iInt(it));
        break;
      case Val_Char:
        *target = *iBytes(it);
        ++len;
        break;
    }
  }
  result[len++] = 0;
  returnCollapseItem(p0, Item(len, Val_Str));
}

void ChVM::op_Type (itemnum p0) {
  *(IType*)iBytes(p0) = i(p0)->type; //TODO: Arduino test
  returnItem(p0, Item(sizeof(IType), fitInt(sizeof(IType))));
}

void ChVM::op_Cast (itemnum p0) {
  //Get IType parameters
  IType from = i(p0)->type;
  IType to   = (IType)iInt(p0 + 1);
  //Zero out new memory, and ensure strings/blobs are cast correctly
  uint8_t oldNByte = i(p0)->len;
  uint8_t newNByte = constByteLen(to);
  if (to == Val_Blob) newNByte = oldNByte;
  if (to == Val_Str) newNByte = oldNByte + 1;
  if (newNByte > oldNByte)
    memset(iBytes(p0) + oldNByte, 0, newNByte - oldNByte);
  //Return new item descriptor
  returnItem(p0, Item(newNByte, to));
}

void ChVM::op_Vec (itemnum p0) {
  //Copy item descriptors onto the end of the byte stack
  uint8_t* descs = stackItem();
  vectlen nItems = numItem() - p0;
  bytenum itemsLen = sizeof(Item) * nItems;
  memcpy(descs, iLast(), itemsLen);
  //Append number of items
  writeNum(descs + itemsLen, nItems, sizeof(vectlen));
  //Return umbrella item descriptor
  bytenum bytesLen = itemsBytesLen(p0, numItem()) + itemsLen + sizeof(vectlen);
  returnItem(p0, Item(bytesLen, Val_Vec));
}

void ChVM::op_Nth (itemnum p0) {
  int32_t nth = iInt(p0);
  Item* it = i(p0 + 1);
  uint8_t* vBytes = iBytes(p0 + 1);
  //Return nil on negative nth or non-str/vec/blob
  IType type = it->type;
  if (nth < 0 || (type != Val_Vec && type != Val_Str && type != Val_Blob)) {
    returnNil(p0);
    return;
  }
  //Different behaviour for if a string, vector, or blob
  if (type == Val_Str) {
    if (it->len < 2 || nth > it->len - 2) {
      returnNil(p0);
      return;
    }
    *iBytes(p0) = vBytes[nth];
    returnItem(p0, Item(1, Val_Char));
    return;
  }
  if (type == Val_Blob) {
    if (it->len == 0 || nth >= it->len) {
      returnNil(p0);
      return;
    }
    *iBytes(p0) = vBytes[nth];
    returnItem(p0, Item(1, Val_U08));
    return;
  }
  //Collate vector info
  uint8_t* vEnd = (vBytes + it->len) - sizeof(vectlen);
  itemnum vNumItem = readNum(vEnd, sizeof(vectlen));
  Item* vItems = &((Item*)vEnd)[-vNumItem];
  //Find item descriptor at nth
  Item* nthItem = &((Item*)vEnd)[-(nth + 1)];
  //Copy bytes into return position
  uint8_t* itemBytes = vBytes;
  for (itemnum vi = 0; vi < nth; ++vi)
    itemBytes += vItems[(vNumItem - 1) - vi].len;
  memcpy(iBytes(p0), itemBytes, nthItem->len);
  //Return nth item descriptor
  returnItem(p0, nthItem);
}

void ChVM::op_Len (itemnum p0) {
  Item* item = i(p0);
  itemlen len = item->len;
  switch (item->type) {
    case Val_Vec: len = vectLen(p0); break;
    case Val_Str: --len; break;
  }
  writeUNum(iBytes(p0), len, sizeof(itemlen));
  returnItem(p0, Item(sizeof(itemlen), fitInt(sizeof(itemlen))));
}

void ChVM::op_Sect (itemnum p0, bool isBurst) {
  IType type = i(p0)->type;
  bool isStr = type == Val_Str;
  //Return nil if not a vector or string
  if (type != Val_Vec && !isStr) {
    returnNil(p0);
    return;
  }
  //Get vector or string length
  vectlen len = isStr ? i(p0)->len - 1 : vectLen(p0);
  //Retain the skip and take, or use defaults
  vectlen skip = 1,
          take = len - skip;
  {
    argnum nArg = numItem() - p0;
    if (nArg > 1) skip = iInt(p0 + 1);
    if (nArg > 2) take = iInt(p0 + 2);
  }
  //Bound skip and take to length
  if (skip >= len) {
    //Return empty vector or string if skip is beyond length
    if (isStr) {
      *(char*)iBytes(p0) = 0;
      returnItem(p0, Item(1, Val_Str));
    } else {
      writeUNum(iBytes(p0), 0, sizeof(vectlen));
      returnItem(p0, Item(sizeof(vectlen), Val_Vec));
    }
    return;
  }
  if (skip + take > len)
    take = len - skip;
  if (isStr) {
    //Copy subsection of memory to start of the string, add terminator
    memcpy(iBytes(p0), iBytes(p0) + skip, take);
    iBytes(p0)[take] = 0;
    //Either return burst characters (b-sect) or a string (sect)
    returnItem(p0, Item(take + 1, Val_Str));
    if (isBurst) burstItem();
    return;
  }
  //Vector becomes only parameter and is burst
  trunStack(p0 + 1);
  burstItem();
  //Truncate to skip+take
  trunStack(p0 + skip + take);
  //Either return burst items (b-sect) or a vector (sect)
  if (isBurst) {
    collapseItems(p0, take);
  } else {
    op_Vec(p0 + skip);
    returnCollapseLast(p0);
  }
}

void ChVM::op_Reduce (itemnum p0) {
  //Extract function or op number from first parameter
  bool isOp = i(p0)->type == Var_Op;
  funcnum fCode = iInt(p0);
  //Burst item in situ (the last item on the stack)
  burstItem();
  //Copy seed or first item onto stack - either (reduce f v) (reduce f s v)
  restackCopy(p0 + 1);
  //Reduce loop, where the stack is now: [burst v]*N [seed: either v0 or seed]
  itemnum iSeed = numItem() - 1;
  for (itemnum it = p0 + 2; it < iSeed; ++it) {
    //Copy next item onto stack
    restackCopy(it);
    //Execute func or op, which returns to iSeed
    if (isOp) nativeOp((IType)fCode, iSeed);
    else      exeFunc(fCode, iSeed);
  }
  //Collapse return
  returnCollapseLast(p0);
}

void ChVM::op_Map (itemnum p0) {
  //Extract function or op number from first parameter
  bool isOp = i(p0)->type == Var_Op;
  funcnum fCode = iInt(p0);
  //Find shortest vector
  itemnum iFirstVec = p0 + 1;
  itemnum nVec = numItem() - iFirstVec;
  vectlen shortest = -1;
  for (itemnum it = iFirstVec, itEnd = iFirstVec + nVec; it < itEnd; ++it) {
    vectlen l = vectLen(it);
    if (l < shortest) shortest = l;
  }
  //Map loop
  itemnum iMapped = p0 + 1 + nVec;
  uint8_t* pFirstVec = iBytes(iFirstVec);
  for (itemnum it = 0; it < shortest; ++it) {
    bytenum descOffset = sizeof(Item) * it;
    for (itemnum vi = iFirstVec, viEnd = iFirstVec + nVec; vi < viEnd; ++vi) {
      //Fast-fetch descriptor and bytes
      //  by using a byte offset in the cannibalised first Item
      uint8_t* vFirstDesc = pFirstVec + ((itemsBytesLen(iFirstVec, vi + 1) - sizeof(vectlen)) - sizeof(Item));
      itemlen viBytesOffset = it ? readUNum(vFirstDesc, sizeof(itemlen))
                                 : itemsBytesLen(iFirstVec, vi);
      Item iDesc = *(Item*)(vFirstDesc - descOffset); //TODO: Arduino test
      if (it)
        writeUNum(vFirstDesc, readUNum(vFirstDesc, sizeof(itemlen)) + iDesc.len, sizeof(itemlen));
      else
        writeUNum(vFirstDesc, viBytesOffset + iDesc.len, sizeof(itemlen));
      //Copy onto stack
      memcpy(stackItem(), pFirstVec + viBytesOffset, iDesc.len);
      stackItem(iDesc);
    }
    if (isOp) nativeOp((IType)fCode, iMapped + it);
    else      exeFunc(fCode, iMapped + it);
  }
  //Vectorise and collapse return
  op_Vec(iMapped);
  returnCollapseLast(p0);
}

void ChVM::op_For (itemnum p0) {
  //Extract function or op number from first parameter
  bool isOp = i(p0)->type == Var_Op;
  funcnum fCode = iInt(p0);
  //Output N byte lengths and N byte counters onto stack as blob item
  itemnum iFirstVec = p0 + 1;
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
      itemlen iLen = desc.len;
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
  op_Vec(p0 + 1 + nVec + 1);
  returnCollapseLast(p0);
}

void ChVM::op_Loop (itemnum p0) {
  argnum nArg = numItem() - p0;
  uint16_t from = 0, to;
  bool isOp;
  funcnum fCode;
  //If (loop times f)
  if (nArg == 2) {
    to = iInt(p0);
    isOp = i(p0 + 1)->type == Var_Op;
    fCode = iInt(p0 + 1);
  }
  //If (loop a b f)
  else {
    from = iInt(p0);
    to = iInt(p0 + 1);
    isOp = i(p0 + 2)->type == Var_Op;
    fCode = iInt(p0 + 2);
  }
  //Prepare stack with one 16-bit int
  for (uint16_t i = from; i < to; ++i) {
    //Copy i to p0
    returnItem(p0, Item(Val_U16));
    writeUNum(iBytes(p0), i, sizeof(i));
    //Execute f
    if (isOp) nativeOp((IType)fCode, p0);
    else      exeFunc(fCode, p0);
  }
  //The last item on the stack is implicitly returned
}

void ChVM::op_Binds (itemnum p0) {
  //Remove non-binding items
  for (itemnum p = p0; p < numItem(); ++p) {
    if (!p) continue;
    if (i(p)->type != Bind_Mark && i(p - 1)->type != Bind_Mark) {
      collapseItems(p, numItem() - p - 1);
      --p;
    }
  }
  //Deduplicate binds
  for (itemnum p = p0; p < numItem() - 2; ++p) {
    //If this is a binding...
    if (i(p)->type == Bind_Mark) {
      //If the binding is found later on, erase this binding
      bindnum bNum = iInt(p);
      itemnum b = p + 2;
      for (; b < numItem() - 1; ++b)
        if (i(b)->type == Bind_Mark && iInt(b) == bNum)
          break;
      if (b + 1 != numItem())
        collapseItems(p, numItem() - p - 2);
    }
  }
  //Vectorise all remaining parameters
  op_Vec(p0);
}

void ChVM::op_Val (itemnum p0) {
  //Truncate the stack to the first item
  trunStack(p0 + 1);
}

void ChVM::op_Do (itemnum p0) {
  //Collapse the stack to the last item
  returnCollapseLast(p0);
}

void ChVM::op_Pub (itemnum p0) {
  //Pre-emptively stack a nil
  stackNil();
  //Publish the message
  prognum savePNum = pNum;
  broker.publish(iStr(p0), i(p0 + 1), iBytes(p0 + 1), this);
  //During a publish, the program may have been switched away from
  //  so switch back to the original, publishing program
  switchToProg(savePNum);
  //If a published message caused another program to publish a message
  //  which this program was subscribed to, in order to not lose state
  //  the state change that occurred in the subscription handler
  //  is returned, otherwise the pre-emptive nil is returned
  returnCollapseLast(p0);
}
void ChVM::op_Sub (itemnum p0) {
  bool provideT = numItem() - p0 > 2;
  broker.subscribe(iStr(p0), pNum, iInt(p0 + 1), provideT);
  returnNil(p0);
}
void ChVM::op_Unsub (itemnum p0) {
  //Either unsubscribe from everything (0 param)
  //  or a particular topic (1 param)
  broker.unsubscribe(pNum, numItem() - p0 ? iStr(p0) : nullptr);
  returnNil(p0);
}

void ChVM::op_MsNow (itemnum p0) {
  auto msNow = harness->msNow();
  writeNum(iBytes(p0), msNow, sizeof(msNow));
  returnItem(p0, Item(fitInt(sizeof(msNow))));
}

void ChVM::op_Sleep (itemnum p0) {
  pInfo->sleepUntil = harness->msNow() + iInt(p0);
  returnNil(p0);
}

void ChVM::op_Print (itemnum p0) {
  op_Str(p0);
  harness->print(iStr(p0));
  returnNil(p0);
}

void ChVM::op_Debug (itemnum p0) {
  uint32_t out = 0;
  switch (iInt(p0)) {
    case 0: out = numItem(); break;
    case 1: out = numByte(); break;
    case 2: harness->printItems(pFirstItem, numItem()); break;
    case 3: harness->printMem(pBytes, numByte());
  }
  writeUNum(iBytes(p0), out, sizeof(out));
  returnItem(p0, Item(fitInt(sizeof(out))));
}

void ChVM::op_Load (itemnum p0) {
  //After loading a program the program context will have been switched
  //  We need to ensure we restore it to this one
  prognum savePNum = pNum;
  memcpy(iBytes(p0) + i(p0)->len - 1, ".kua\0", 5);
  bool success = harness->loadProg(iStr(p0));
  switchToProg(savePNum);
  returnBool(p0, success);
}

void ChVM::op_Halt () {
  //Halt all execution while unrolling ChVM:: exeForm & exeFunc
  funcState = Halted;
  pInfo->isHalting = true;
}
