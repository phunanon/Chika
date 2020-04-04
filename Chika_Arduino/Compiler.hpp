#pragma once
#include "config.hpp"
#include "ChVM_Harness.hpp"

class Appender;
class Streamer;
struct idx;
struct Counters;
struct Params;

class Compiler {
  ChVM_Harness* h;
  void concatFiles(const char*, const char*);
  void hashOut (const char*, const char*);
  bindnum newBind (const char*);
  idx findHash (const char*, const char*);
  void compileFunc (funcnum, Counters&, Streamer&, Appender&);
  void compileForm (Params*, Counters&, Streamer&, Appender&);
  void compileArg  (Params*, Counters&, Streamer&, Appender&);
public:
  Compiler (ChVM_Harness*);
  void compile (const char*, const char*);
};