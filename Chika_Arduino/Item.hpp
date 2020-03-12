#pragma once
#include <stdint.h>
#include "config.hpp"

class __attribute__((__packed__)) Item {
  uint8_t typeAndKind;
public:
  itemlen len; //Length of value or const value
  Item (itemlen, IType, bool);
  Item (itemlen, IType);
  Item (IType);
  IType type ();
  bool isConst ();
};
