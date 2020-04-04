#ifdef ARDUINO
#include <avr/pgmspace.h>
#endif

#ifdef ARDUINO
const char* const ops[] PROGMEM = {
#else
const char* const ops[] = {
#endif
  "if", "case", "or", "and", "not",             //23-27
  "return", "recur",                            //28-29
  "=", "==", "!=", "!==", "<", "<=", ">",       //2A-30
  ">=", "+", "-", "*", "/", "mod", "pow",       //31-37
  "~", "&", "|", "^", "<<", ">>",               //38-3D
  "p-mode", "dig-r", "dig-w", "ana-r", "ana-w", //3E-42
  "file-r", "file-w", "file-a", "file-d",       //43-46
  "str", "vec", "nth", "len", "sect", "b-sect", //47-4C
  "..", "reduce", "map", "for", "loop",         //4D-51
  "binds", "val", "do",                         //52-54
  "pub", "sub", "unsub",                        //
  "type", "cast",                               //
  "ms-now", "sleep",                            //
  "print", "debug", "load", "comp", "halt", 0   //
};

#ifdef ARDUINO
const char* const fCodes[] PROGMEM = {
#else
const char* const fCodes[] = {
#endif
"if", "or", "and", "case", 0};