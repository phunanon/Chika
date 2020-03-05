#pragma once

class ChVM_Harness {
public:
  //For debugging
  void     print      (const char*);
  void     printInt   (const char*, uint32_t);
  void     printMem   (uint8_t*, uint8_t);
  void     printItems (uint8_t*, uint32_t);

  //File and General-Purpose IO
  int32_t  fileRead   (const char*, uint8_t*, uint32_t, uint32_t);
  bool     fileWrite  (const char*, uint8_t*, uint32_t, uint32_t);
  bool     fileAppend (const char*, uint8_t*, uint32_t);

  //Program control
  void     loadProg   (const char*);
  void     unloadProg (const char*);

  //System information
  uint32_t msNow      ();
};
