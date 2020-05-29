#include "Compiler.hpp"
#include "CompileTables.hpp"
#include "utils.hpp"

typedef uint8_t buflen;

//FIXME: when Arduino supports C++14
//  perhaps update the compiler to use a closure
class Appender {
  ChVM_Harness* h;
  uint8_t buffer [32] = {0};
  uint8_t buffLen = 0;

public:
  const char* path;
  proglen nWritten = 0;

  Appender (ChVM_Harness* harness, const char* pathOut)
    : h(harness), path(pathOut) {}
  
  ~Appender () {
    flushBuffer();
  }

  void a (uint8_t* data, uint8_t len) {
    if (buffLen + len > sizeof(buffer))
      flushBuffer();
    if (len > sizeof(buffer))
      h->fileAppend(path, data, len);
    else {
      memcpy(buffer + buffLen, data, len);
      buffLen += len;
    }
    nWritten += len;
  }

  void flushBuffer () {
    h->fileAppend(path, buffer, buffLen);
    buffLen = 0;
  }
};

//Crude streaming logic, with buffer and lookahead
//  Transparently buffers at least nLookahead bytes
class Streamer {
  static const buflen bufferLen = 128;
  static const uint8_t nLookahead = 4;
  ChVM_Harness* _h;
  const char* _path;
  proglen fSize = 0;
  proglen fOffset = 0;
  buflen bOffset = 0;
  uint8_t lookaheads[nLookahead];

  //Read into buffer, leaving terminating \0
  void fillBuffer () {
    _h->fileRead(_path, (uint8_t*)buffer, fOffset, sizeof(buffer) - 1);
  }

  //Redo lookaheads and make next buffer if necessary
  void reLookahead () {
    if (bOffset >= bufferLen - nLookahead)
      refreshBuffer();
    memcpy(lookaheads, buffer + bOffset, nLookahead);
  }
public:
  uint8_t buffer [bufferLen] = {0};

  Streamer (ChVM_Harness* harness, const char* path, buflen skip = 0)
    : _h(harness), _path(path), fOffset(skip)
  {
    bOffset = -1; //To enable an immediate .next()
    fSize = harness->fileSize(path);
    fillBuffer();
  }
  
  buflen offset () { return fOffset + bOffset; }
  buflen maxRead () { return min(bufferLen - 1, fSize - fOffset); }
  buflen remaining () {
    proglen o = fOffset + bOffset;
    return o > fSize ? 0 : fSize - o;
  }

  void skip (buflen by = 1) {
    bOffset += by;
    reLookahead();
  }

  uint8_t* peek (uint8_t by = 0) {
    return &buffer[bOffset + by];
  }

  void refreshBuffer () {
    if (fSize - fOffset < bufferLen)
      return;
    fOffset += bOffset;
    bOffset = 0;
    fillBuffer();
  }

  bool next () {
    ++bOffset;
    reLookahead();
    return fOffset + bOffset >= fSize;
  }

  uint8_t &operator[] (uint8_t i) {
    return lookaheads[i];
  }
};


bool isWhitespace (char ch) {
  return ch == ',' || ch == ' ' || ch == '\n' || ch == ';';
}

bool patternIs (uint8_t* m, const char* str) {
  return !memcmp(m, str, strlen(str));
}

typedef uint32_t hash32;
hash32 hash (const char* str) {
  if (!str) return 0;
  uint32_t hash = 5381;
  while (*str)
    hash = ((hash << 5) + hash) + *str++;
  return hash;
}

struct idx {
  uint16_t i;
  uint8_t found;
  idx (uint16_t _i, uint8_t _found) : i(_i), found(_found) {}
};

idx indexOf (uint8_t* str, char ch) {
  buflen i = 0;
  while (*str != ch && *str) {
    ++i;
    ++str;
  }
  return idx(i, *str);
}

idx firstNonSymbol (uint8_t* str) {
  buflen i = 0;
  while (*str && !isWhitespace(*str) && *str != '(' && *str != ')') {
    ++i;
    ++str;
  }
  return idx(i, *str);
}

idx firstNull (uint8_t* str, buflen max) {
  for (buflen i = 0; i < max; ++i)
    if (!*str++)
      return idx(i, true);
  return idx(0, false);
}



Compiler::Compiler (ChVM_Harness* harness)
  : h(harness) {}

struct Counters {
  proglen strOffset = 0;
  uint8_t nCha = 0;
};

void Compiler::hashOut (const char* file, const char* symbol) {
  hash32 hsh = hash(symbol);
  h->fileAppend(file, (uint8_t*)&hsh, sizeof(hsh));
}

void Compiler::concatFiles (const char* into, const char* addition) {
  if (h->fileSize(addition) == -1) return;
  auto out = Appender(h, into);
  auto s = Streamer(h, addition);
  s.next();
  while (s.remaining()) {
    out.a((uint8_t*)s.peek(), s.maxRead());
    s.skip(s.maxRead());
  }
}

void Compiler::cleanUp () {
  //Delete any possible residue files from a previous unsuccessful compilation
  h->fileDelete("sexpr.chc");
  h->fileDelete("inlines.chc");
  h->fileDelete("ifuncs.hsh");
  h->fileDelete("clean.chc");
  h->fileDelete("entry.chc");
  h->fileDelete("heart.chc");
  h->fileDelete("body.chc");
  h->fileDelete("strings.chc");
  h->fileDelete("chars.chc");
  h->fileDelete("stripped.chc");
  h->fileDelete("funcs.hsh");
  h->fileDelete("binds.hsh");
}

void Compiler::compile (const char* pathIn, const char* pathOut) {
  h->fileDelete(pathOut);
  cleanUp();

  //Maybe remove shebang and/or maybe extract RAM request
  proglen ramRequest = MAX_PROG_RAM;
  proglen afterShebang;
  {
    auto s = Streamer(h, pathIn);
    while (true) {
      if (s.next()) break;
      if (s[0] == '#') {
        buflen after = indexOf(s.peek(), '\n').i;
        if (isDigit(s[1]))
          ramRequest = chars2int((const char*)s.peek(1));
        s.skip(after);
      } else {
        afterShebang = s.offset();
        break;
      }
    }
  }
  
  
  auto litTransparency = [] (Streamer &s, Appender &a, bool &inString) {
    //Check if we're within/exiting a string
    if (inString) {
      if (s[0] == '"') inString = false;
      a.a(&s[0], 1);
      return true;
    }
    //Check if entering a string or a char
    if (s[0] == '"')
      inString = true;
    if (s[0] == '\\') {
      uint8_t chLen = s[2] == ' ' ? 3 : 2;
      a.a(s.peek(), chLen);
      s.skip(chLen);
    }
    return false;
  };

  //Comment, comma, semi-colon, newline, excessive whitespace removal
  //  [] -> (vec ), transparent string/char pass-through
  //Single-character step
  {
    auto s = Streamer(h, pathIn, afterShebang);
    auto cleanOut = Appender(h, "clean.chc");
    bool inComment = false,
         isMulti = false,
         inComma = false,
         inString = false;
    char prevCh = '\0';
    while (true) {
      if (s.next()) break;

      //Ignoring comments until break
      if (inComment) {
        if (!isMulti && s[0] == '\n')
          inComment = false;
        if (isMulti && (s[0] == '*' && s[1] == '/')) {
          inComment = false;
          s.skip(2);
        }
        continue;
      }
      
      //Ignoring whitespace after commas until break
      if (inComma && !isWhitespace(s[0]))
        inComma = false;
      if (inComma) continue;

      //String and character transparency
      if (litTransparency(s, cleanOut, inString)) continue;

      if (s[0] == '[') {
        const char* vec = "(vec ";
        cleanOut.a((uint8_t*)vec, 5);
        continue;
      }
      if (s[0] == ']') {
        char paren = ')';
        cleanOut.a((uint8_t*)&paren, 1);
        continue;
      }

      //Check if entering a comment
      if (s[0] == '/' && (s[1] == '/' || s[1] == '*')) {
        inComment = true;
        isMulti = s[1] == '*';
        continue;
      }

      bool isWhite = isWhitespace(s[0]);
      if (isWhite)
        inComma = s[0] == ',';
      if (!(isWhitespace(s[0]) && isWhitespace(prevCh))) {
        if (s[0] == '\n') s[0] = ' ';
        prevCh = s[0];
        cleanOut.a(&s[0], 1);
      }
    }
  }

  //Restructure clean.chc so it goes: entry forms (as a func), heartbeat func, rest
  {
    auto s = Streamer(h, "clean.chc");
    auto entryOut = Appender(h, "entry.chc");
    auto heartOut = Appender(h, "heart.chc");
    auto bodyOut = Appender(h, "body.chc");
    bool inHeartbeat = false, inEntry = false, inString = false;
    uint8_t depth = 0;
    while (true) {
      if (s.next()) break;
      //When at the top s-expression level, check for expression type
      if (!depth) {
        s.refreshBuffer();
        if (patternIs(s.peek(), "(fn heartbeat "))
          inHeartbeat = true;
        if (!patternIs(s.peek(), "(fn "))
          inEntry = true;
      }
      Appender* a = (inHeartbeat ? &heartOut : (inEntry ? &entryOut : &bodyOut));
      //String and character transparency
      if (litTransparency(s, *a, inString)) continue;
      if (s[0] == '(') ++depth;
      if (s[0] == ')') --depth;
      a->a((uint8_t*)&s[0], 1);
      if (!depth)
        inHeartbeat = inEntry = false;
    }
    //If no heartbeat was written, output a halting one
    if (!heartOut.nWritten)
      heartOut.a((uint8_t*)"(fn heartbeat (halt))", 21);
  }
  //Concat entry.chc + heart.chc + body.chc = clean.chc
  {
    auto cleanOut = Appender(h, "clean.chc");
    h->fileDelete("clean.chc");
    cleanOut.a((uint8_t*)"(fn entry ", 10);
    cleanOut.flushBuffer();
    concatFiles("clean.chc", "entry.chc");
    cleanOut.a((uint8_t*)")", 1);
    cleanOut.flushBuffer();
    concatFiles("clean.chc", "heart.chc");
    concatFiles("clean.chc", "body.chc");
    h->fileDelete("entry.chc");
    h->fileDelete("heart.chc");
    h->fileDelete("body.chc");
  }


  //Extract and hash inline functions
  //  and hash regular functions
  {
    auto s = Streamer(h, "clean.chc");
    auto sexprOut = Appender(h, "sexpr.chc");
    auto inlinesOut = Appender(h, "inlines.chc");
    bool inString = false;
    funcnum iFNum = 1111;
    while (true) {
      if (s.next()) break;
      if (patternIs(s.peek(), "(fn ")) {
        s.refreshBuffer();
        auto symLen = firstNonSymbol(s.peek(4)).i;
        char after = *s.peek(4 + symLen);
        *s.peek(4 + symLen) = '\0';
        hashOut("funcs.hsh", (const char*)s.peek(4));
        *s.peek(4 + symLen) = after;
      }
      //String and character transparency
      if (litTransparency(s, sexprOut, inString)) continue;
      if (s[0] == '{') {
        s.refreshBuffer();
        const char* fHead0 = "(fn ";
        uint8_t fNumChars[6] = {0};
        fNumChars[0] = 'F';
        int2chars(fNumChars + 1, iFNum++);
        hashOut("ifuncs.hsh", (const char*)fNumChars);
        const char* fHead1 = " (";
        const char* fTail = "))";
        inlinesOut.a((uint8_t*)fHead0, 4);
        inlinesOut.a(fNumChars, sizeof(fNumChars) - 1);
        inlinesOut.a((uint8_t*)fHead1, 2);
        while (true) {
          if (s.next()) break;
          if (s[0] == '}') break;
          //String and character transparency
          if (litTransparency(s, inlinesOut, inString)) continue;
          inlinesOut.a(&s[0], 1);
        }
        inlinesOut.a((uint8_t*)fTail, 2);
        sexprOut.a(fNumChars, sizeof(fNumChars) - 1);
        continue;
      }
      if (s[0] != '\n')
        sexprOut.a(&s[0], 1);
    }
    h->fileDelete("clean.chc");
  }
  //Append inline functions back into sexpr.chc
  //  and append inline function hashes into funcs.hsh
  concatFiles("sexpr.chc", "inlines.chc");
  concatFiles("funcs.hsh", "ifuncs.hsh");
  h->fileDelete("inlines.chc");
  h->fileDelete("ifuncs.hsh");

  //String and char extraction/stripping
  {
    auto s = Streamer(h, "sexpr.chc");
    auto strippedOut = Appender(h, "stripped.chc");
    auto charsOut = Appender(h, "chars.chc");
    auto stringsOut = Appender(h, "strings.chc");
    bool inString = false;
    while (true) {
      if (s.next()) break;
      if (s[0] == '"') {
        inString = !inString;
        if (!inString) {
          strippedOut.a((uint8_t*)"\" ", s[1] != ' ' ? 2 : 1);
          s.skip();
          stringsOut.a((uint8_t*)"\0", 1);
        } else {
          continue;
        }
      }
      if (!inString && s[0] == '\\') {
        uint8_t literal;
        if (s[1] == 'n' && s[2] == 'l') {
          literal = '\n';
          s.skip(3);
        } else if (s[1] == 's' && s[2] == 'p') {
          literal = ' ';
          s.skip(3);
        } else {
          literal = s[1];
          s.skip(2);
        }
        charsOut.a(&literal, 1);
        strippedOut.a((uint8_t*)"\\ ", 2);
      }
      (inString ? stringsOut : strippedOut).a(&s[0], 1);
    }
    h->fileDelete("sexpr.chc");
  }
  



  {
    auto binOut = Appender(h, pathOut);
    //Output dummy RAM request
    binOut.a((uint8_t*)"  ", 2);
    //Serialise s-expressions
    auto s = Streamer(h, "stripped.chc");
    s.next();
    funcnum fN = 0;
    auto counters = Counters();
    while (true) {
      //Skip the end of a level-0 func or form
      if (s[0] == ')' || !s[0]) {
        if (s.next())
          break;
        continue;
      }
      s.refreshBuffer();
      compileFunc(fN++, counters, s, binOut);
    }
  }


  //Output real RAM request plus length of compiled binary
  {
    ramRequest += h->fileSize(pathOut) - sizeof(ramRequest);
    uint8_t rReq[sizeof(ramRequest)];
    memcpy(rReq, &ramRequest, sizeof(ramRequest));
    h->fileWrite(pathOut, rReq, 0, sizeof(rReq));
  }

  cleanUp();
}


struct Params {
  uint32_t hashes[16];
  uint8_t n = 0;
  idx contains (const char* symbol) {
    auto compare = hash(symbol);
    for (uint8_t h = 0; h < n; ++h)
      if (hashes[h] == compare)
        return idx(h, true);
    return idx(0, false);
  }
  void add (const char* symbol) {
    hashes[n++] = hash(symbol);
  }
  uint32_t &operator[] (uint8_t i) {
    return hashes[i];
  }
};

void Compiler::compileFunc (funcnum fNum, Counters& c, Streamer& s, Appender& binOut) {
  s.skip(); //Skip first (
  //Skip "fn name "
  {
    bool isBlank = false;
    s.skip(3);
    auto symLen = firstNonSymbol(s.peek()).i;
    if (*s.peek(symLen) == ')')
      isBlank = true;
    s.skip(symLen + 1);
    //If this is a blank function do not output any bytecode
    //  The VM will return nil for functions it cannot find
    while (s[0] == ' ') s.skip();
    if (isBlank || s[0] == ')') return;
  }
  //Add params to a hash table
  auto params = Params();
  if (s[0] != '(') {
    while (true) {
      if (s[0] == '(') break;
      auto symLen = firstNonSymbol(s.peek()).i;
      *s.peek(symLen) = '\0';
      params.add((const char*)s.peek());
      s.skip(symLen + 1);
    }
  }
  //Output function number and length placeholder
  uint8_t fNBytes[sizeof(fNum)];
  writeUNum(fNBytes, fNum++, sizeof(fNum));
  binOut.a(fNBytes, sizeof(fNBytes));
  binOut.flushBuffer();
  proglen writeFLenAt = h->fileSize(binOut.path);
  binOut.a((uint8_t*)"  ", 2);

  binOut.nWritten = 0;
  //Compile form/s
  while (s[0] != ')') {
    if (s[0] == ' ') {
      s.skip();
      continue;
    }
    compileForm(&params, c, s, binOut);
  }

  //Prepend function length
  binOut.flushBuffer();
  h->fileWrite(binOut.path, (uint8_t*)&binOut.nWritten, writeFLenAt, sizeof(funclen));
}


idx Compiler::findHash (const char* file, const char* symbol) {
  if (h->fileSize(file) == -1) return idx(0, false);
  hash32 hsh = hash(symbol);
  auto s = Streamer(h, file);
  s.next();
  for (funcnum f = 0; s.remaining(); ++f) {
    hash32 testHash;
    memcpy(&testHash, s.peek(), sizeof(testHash));
    s.skip(sizeof(testHash));
    if (testHash == hsh)
      return idx(f, true);
  }
  return idx(0, false);
}

IType symToOp (const char* symbol) {
  for (uint8_t o = 0; ops[o]; ++o)
    if (!strcmp(symbol, ops[o])) {
      return (IType)(o + OPS_START + 1); //+1 to skip Op_Func
    }
  return (IType)0;
}

bindnum Compiler::newBind (const char* symbol) {
  hashOut("binds.hsh", symbol);
  return (h->fileSize("binds.hsh") / sizeof(hash32)) - 1;
}

void Compiler::compileForm (Params* p, Counters& c, Streamer& s, Appender& binOut) {
  s.skip(); //Skip '('
  s.refreshBuffer();
  while (s[0] == ' ') s.skip();
  //The beginning of every form is an op, so serialise this first
  //  which could be an operation, function, func-through-parm, or func-through-bind
  auto symLen = firstNonSymbol(s.peek()).i;
  bool isOpOnly = *s.peek(symLen) == ')';
  *s.peek(symLen) = '\0';
  const char* symbol = (const char*)s.peek();
  s.skip(symLen + 1);
  //Serialise op code
  uint8_t opSerialised[3] = {0};
  uint8_t opSerialisedLen = 1;
  {
    //Check if it's a native op
    IType op = symToOp(symbol);
    if (op) opSerialised[0] = op;
    //If it wasn't a native op, check if a prog func
    if (!opSerialised[0]) {
      idx fNum = findHash("funcs.hsh", symbol);
      if (fNum.found) {
        opSerialised[0] = Op_Func;
        writeUNum(opSerialised + 1, fNum.i, sizeof(funcnum));
        opSerialisedLen = 1 + sizeof(funcnum);
      }
    }
    //If it wasn't a native op or prog func, check if a param
    if (!opSerialised[0]) {
      idx pNum = p->contains(symbol);
      if (pNum.found) {
        opSerialised[0] = Op_Param;
        writeUNum(opSerialised + 1, pNum.i, sizeof(argnum));
        opSerialisedLen = 1 + sizeof(argnum);
      }
    }
    //If it wasn't a native op, prog func, or param, it must be a binding
    if (!opSerialised[0]) {
      idx bNum = findHash("binds.hsh", symbol);
      //If a binding was found, use it as the function
      //  otherwise this can only be treated as the first
      //  instance of a binding, and must be hashed
      if (!bNum.found)
        bNum.i = newBind(symbol);
      opSerialised[0] = Op_Bind;
      writeUNum(opSerialised + 1, bNum.i, sizeof(bindnum));
      opSerialisedLen = 1 + sizeof(bindnum);
    }
  }
  //Output form code
  {
    IType fCode = Form_Eval;
    for (uint8_t c = 0; fCodes[c]; ++c)
      if (!strcmp(symbol, fCodes[c])) {
        fCode = (IType)(c + 1); //+1 to skip Form_Eval
        break;
      }
    binOut.a((uint8_t*)&fCode, 1);
  }
  //Process arguments recursively
  //  until our ')'
  if (!isOpOnly) {
    while (s[0] != ')') {
      if (s[0] == ' ') {
        s.skip();
        continue;
      }
      if (s[0] == '(')
        compileForm(p, c, s, binOut);
      else
        compileArg(p, c, s, binOut);
    }
    s.skip(); //Skip the ')'
  }
  //Output the serialised op
  binOut.a(opSerialised, opSerialisedLen);
}


void tOut (IType t, Appender& binOut) {
  binOut.a((uint8_t*)&t, sizeof(t));
}

void Compiler::compileArg (Params* p, Counters& c, Streamer& s, Appender& binOut) {
  s.refreshBuffer();
  //Determine symbol
  auto symLen = firstNonSymbol(s.peek()).i;
  char afterSym = *s.peek(symLen);
  *s.peek(symLen) = '\0';
  const char* symbol = (const char*)s.peek();
  //Serialise the symbol which could be a
  //  boolean, nil, char, string, args emit, numbered param,
  //  integer, named param, func ref, op ref, bindings
  bool isSerialised = false;
  //If a boolean or nil
  if (symLen == 1 && (s[0] == 'T' || s[0] == 'F' || s[0] == 'N')) {
    isSerialised = true;
    tOut(s[0] == 'T' ? Val_True : (s[0] == 'F' ? Val_False : Val_Nil), binOut);
  }
  //If a char
  if (s[0] == '\\') {
    isSerialised = true;
    uint8_t ch;
    h->fileRead("chars.chc", &ch, c.nCha++, 1);
    tOut(Val_Char, binOut);
    binOut.a(&ch, 1);
  }
  //If a string
  if (!isSerialised && s[0] == '"') {
    isSerialised = true;
    tOut(Val_Str, binOut);
    char strCache[128];
    h->fileRead("strings.chc", (uint8_t*)strCache, c.strOffset, sizeof(strCache));
    strilen sLen = strlen(strCache) + 1;
    binOut.a((uint8_t*)strCache, sLen);
    c.strOffset += sLen;
  }
  //If an args emit
  if (!isSerialised && patternIs(s.peek(), "args")) {
    isSerialised = true;
    tOut(Val_Args, binOut);
  }
  //If a numbered parameter
  if (!isSerialised && (s[0] == '#' || s[0] == '$')) {
    isSerialised = true;
    argnum pN = 0;
    if (isDigit(s[1]))
      pN = chars2int((const char*)s.peek(1));
    tOut(s[0] == '#' ? Para_Val : XPara_Val, binOut);
    binOut.a((uint8_t*)&pN, sizeof(pN));
  }
  //If a hexademical or decimal integer
  if (!isSerialised && (isDigit(s[0]) || s[0] == '-')) {
      isSerialised = true;
    bool isDecimal = s[1] != 'x';
    if (patternIs((uint8_t*)symbol, "0x") || isDecimal) {
      uint8_t size;
      char tail = symbol[symLen - 1];
      if (isDecimal)
        size = tail == 'w' ? 2 : (tail == 'i' ? 4 : 1);
      else {
        uint8_t len = strlen(symbol);
        size = (len == 5 || len == 6) ? 2 : (len > 6 ? 4 : 1);
      }
      tOut(size == 2 ? Val_U16 : (size == 4 ? Val_I32 : Val_U08), binOut);
      uint8_t intBytes[sizeof(int32_t)];
      writeNum(intBytes, chars2int(symbol + (isDecimal ? 0 : 2), !isDecimal), size);
      binOut.a(intBytes, size);
    }
  }
  //If a named parameter
  if (!isSerialised && p) {
    idx isParam = p->contains(symbol);
    if (isParam.found) {
      isSerialised = true;
      tOut(Para_Val, binOut);
      binOut.a((uint8_t*)&isParam.i, sizeof(argnum));
    }
  }
  //If a function reference
  if (!isSerialised) {
    idx fNum = findHash("funcs.hsh", symbol);
    if (fNum.found) {
      isSerialised = true;
      tOut(Var_Func, binOut);
      binOut.a((uint8_t*)&fNum.i, sizeof(fNum.i));
    }
  }
  //If an op reference
  if (!isSerialised) {
    IType op = symToOp(symbol);
    if (op) {
      isSerialised = true;
      tOut(Var_Op, binOut);
      tOut(op, binOut);
    }
  }
  //Lastly, it must a binding
  if (!isSerialised) {
    isSerialised = true;
    //Determine if a mark or a reference
    //  and if a reference, if extended or regular
    bool isMark = *s.peek(symLen - 1) == '=';
    bool isX = s[0] == '.';
    bool isR = s[0] == '*';
    if (isMark)
      *s.peek(symLen - 1) = '\0';
    tOut(isMark ? Bind_Mark : (isX ? XBind_Val : (isR ? RBind_Val : Bind_Val)), binOut);
    //Either find its existing hash or assign a new one
    symbol += isX + isR; //Skip . or *
    idx bNum = findHash("binds.hsh", symbol);
    if (!bNum.found)
      bNum.i = newBind(symbol);
    binOut.a((uint8_t*)&bNum.i, sizeof(bNum.i));
  }
  if (!isSerialised) {
    //TODO: emit error
  }
  //Skip the symbol
  *s.peek(symLen) = afterSym;
  s.skip(symLen);
}