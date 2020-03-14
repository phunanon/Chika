#pragma once

class ChVM_Harness {
public:
  //For debugging
  void     print      (const char*);
  void     printInt   (const char*, uint32_t);
  void     printMem   (uint8_t*, uint8_t);
  void     printItems (uint8_t*, uint32_t);

  //File and General-Purpose IO
  void     pinMod     (uint8_t, bool);
  bool     digIn      (uint8_t);
  void     digOut     (uint8_t, bool);
  uint16_t anaIn      (uint8_t);
  void     anaOut     (uint8_t, uint16_t);
  int32_t  fileRead   (const char*, uint8_t*, uint32_t, uint32_t);
  bool     fileWrite  (const char*, uint8_t*, uint32_t, uint32_t);
  bool     fileAppend (const char*, uint8_t*, uint32_t);
  bool     fileDelete (const char*);

  //Program control
  bool     loadProg   (const char*);
  void     unloadProg (const char*);

  //System information
  uint32_t msNow      ();
};
