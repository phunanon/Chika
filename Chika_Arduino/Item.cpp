#include "Item.hpp"

Item::Item (itemlen _len, IType _type) {
  len = _len;
  type = _type;
}

Item::Item (IType _type) {
  len = constByteLen(_type);
  type = _type;
}