#pragma once
#include "config.hpp"
#include "Item.hpp"

class ChVM;

struct Sub {
  prognum p;
  funcnum f;
  sublen tLen;
};

class Broker {
  char    topics[SUBS_SIZE];
  Sub     subs[MAX_SUBS];
  strilen topicsLen = 0;
  subnum  numSub = 0;
public:
  Broker ();
  void publish        (const char*, Item*, uint8_t*, ChVM*);
  void subscribe      (const char*, prognum, funcnum);
  void unsubscribe    (prognum, const char* = nullptr);
  void shiftCallbacks (prognum);
};