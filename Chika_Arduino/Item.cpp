#include "Item.hpp"

Item::Item (itemlen _len, IType type, bool isConst) {
  len = _len;
  typeAndKind = (uint8_t)(isConst << 7) | (uint8_t)(type & 0x7F);
}

Item::Item (itemlen _len, IType type) {
  len = _len;
  typeAndKind = (uint8_t)(type & 0x7F);
}

Item::Item (IType type) {
  len = constByteLen(type);
  typeAndKind = (uint8_t)(type & 0x7F);
}

IType Item::type () {
  return (IType)(typeAndKind & 0x7F);
}

bool Item::isConst () {
  return typeAndKind & 0x80;
}
