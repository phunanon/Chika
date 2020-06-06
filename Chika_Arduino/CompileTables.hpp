#ifdef ARDUINO
#include <avr/pgmspace.h>
#endif

#ifdef ARDUINO
const char* const ops[] PROGMEM = {
#else
const char* const ops[] = {
#endif
  "if", "case", "or", "&&", "!",                //23-27
  "return", "recur",                            //28-29
  "=", "==", "!=", "!==", "<", "<=", ">",       //2A-30
  ">=", "+", "-", "*", "/", "%", "**",          //31-37
  "~", "&", "|", "^", "<<", ">>",               //38-3D
  "p-mode", "dig-r", "dig-w", "ana-r", "ana-w", //3E-42
  "file-r", "file-w", "file-a", "file-d",       //43-46
  "str", "vec", "nth", "len", "sect", "..sect", //47-4C
  "blob", "get", "set",                         //4D-4F
  "..", "reduce", "map", "for", "loop",         //50-54
  "binds", "val", "do",                         //55-57
  "pub", "sub", "unsub",                        //58-5A
  "type", "cast",                               //5B-5C
  "ms-now", "sleep", "print", "rand",           //5D-60
  "debug", "load", "comp", "halt", 0            //61-64
};

#ifdef ARDUINO
const char* const fCodes[] PROGMEM = {
#else
const char* const fCodes[] = {
#endif
"if", "or", "&&", "case", 0};