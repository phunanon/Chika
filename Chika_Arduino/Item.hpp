#pragma once
#include <stdint.h>
#include "config.hpp"

class __attribute__((__packed__)) Item {
public:
  IType type;
  itemlen len; //Length of value or const value
  Item (itemlen, IType);
  Item (IType);
};
