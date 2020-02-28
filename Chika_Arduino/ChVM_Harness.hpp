#pragma once

class ChVM_Harness {
public:
  void     print      (const char*);
  void     debugger   (const char*, bool, uint32_t);
  void     printMem   (uint8_t*, uint8_t);
  void     printItems (uint8_t*, uint32_t);
  uint8_t  loadProg   (const char*);
  uint8_t  unloadProg (const char*);
  uint32_t msNow      ();
};
