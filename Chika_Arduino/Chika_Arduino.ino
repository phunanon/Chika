#include <SPI.h>
#include <SD.h>
#include "ChVM.hpp"

#if USE_SERIAL
void ChVM_Harness::print (const char* output) {
  Serial.print(output);
}

void ChVM_Harness::printInt (const char* output, uint32_t number) {
  Serial.print(output);
  Serial.print(" 0x");
  Serial.print(number, HEX);
  Serial.print(" ; ");
  Serial.println(number);
}

void ChVM_Harness::printMem (uint8_t* mem, uint8_t by) {
  uint8_t left = by / 2;
  for (uint8_t i = 0; i < left; ++i) {
    Serial.print((uint32_t)(intptr_t)((mem - left) + i) % 16, HEX);
    Serial.print(" ");
  }
  Serial.print(". ");
  for (uint8_t i = 1; i < by; ++i) {
    Serial.print((uint32_t)(intptr_t)(mem + i) % 16, HEX);
    Serial.print(" ");
  }
  Serial.println();
  uint8_t* mEnd = mem + by;
  for (uint8_t* m = mem - left; m < mEnd; ++m) {
    if (*m < 16) Serial.print("0");
    Serial.print(*m, HEX);
  }
  Serial.println();
}

void ChVM_Harness::printItems (uint8_t* pItems, uint32_t n) {
  Serial.print("Items: ");
  for (uint8_t it = 0; it < n; ++it) {
    Item* item = (Item*)(pItems - (it * sizeof(Item)));
    Serial.print(it);
    Serial.print("l");
    Serial.print(item->len);
    Serial.print("t");
    Serial.print(item->type, HEX);
    Serial.print(" ");
  }
  Serial.println();
}
#endif
#if !USE_SERIAL
void ChVM_Harness::print (const char* output) {}
void ChVM_Harness::printInt (const char* output, uint32_t number) {}
void ChVM_Harness::printMem (uint8_t* mem, uint8_t by) {}
void ChVM_Harness::printItems (uint8_t* pItems, uint32_t n) {}
#endif

void ChVM_Harness::pinMod (uint8_t pin, bool mode) {
  pinMode(pin, mode);
}
bool ChVM_Harness::digIn (uint8_t pin) {
  return digitalRead(pin);
}
void ChVM_Harness::digOut (uint8_t pin, bool val) {
  digitalWrite(pin, val ? HIGH : LOW);
}
uint16_t ChVM_Harness::anaIn (uint8_t pin) {
  return analogRead(pin);
}
void ChVM_Harness::anaOut (uint8_t pin, uint16_t val) {
  analogWrite(pin, val);
}


//Reads from `offset` in the file for `count` bytes
//If `count` is 0 the rest of the file is read
int32_t ChVM_Harness::fileRead (const char* path, uint8_t* blob, uint32_t offset, uint32_t count) {
  File fp = SD.open(path, FILE_READ);
  if (!fp) return 0;
  auto fLen = fp.size();
  if (offset + count > fLen) {
    if (offset > fLen) return 0;
    count = 0;
  }
  if (!count) count = fLen - offset;
  fp.seek(offset);
  fp.read(blob, count);
  fp.close();
  return count;
}

bool ChVM_Harness::fileWrite (const char* path, uint8_t* blob, uint32_t offset, uint32_t count) {
  File fp = SD.open(path, FILE_WRITE);
  if (!fp) return false;
  fp.seek(offset);
  fp.write(blob, count);
  fp.close();
  return true;
}

bool ChVM_Harness::fileAppend (const char* path, uint8_t* blob, uint32_t count) {
  File fp = SD.open(path, FILE_WRITE | O_APPEND);
  if (!fp) return false;
  fp.write(blob, count);
  fp.close();
  return true;
}

bool ChVM_Harness::fileDelete (const char* path) {
  return SD.remove(path);
}


uint32_t ChVM_Harness::msNow () {
  return millis();
}

ChVM_Harness harness = ChVM_Harness();
ChVM machine = ChVM(&harness);

bool ChVM_Harness::loadProg (const char* path) {
  File prog = SD.open(path);
  if (!prog) {
    Serial.println("Program not found");
    return false;
  }
  bytenum memLen;
  {
    char memRequest[4];
    for (uint8_t b = 0; b < sizeof(memLen); ++b)
      memRequest[b] = prog.read();
    memcpy(&memLen, memRequest, sizeof(memLen));
  }
  machine.memLen(memLen);
  machine.setPNum(machine.numProg++);
  uint16_t pByte = 0;
  while (prog.available())
    machine.pROM[pByte++] = prog.read();
  prog.close();
  machine.romLen(pByte);
  machine.entry();
  return true;
}

void setup() {
  bool sdInited = SD.begin(SD_CARD_PIN);
  pinMode(LED_BUILTIN, OUTPUT);
#if USE_SERIAL
  while (!Serial);
  Serial.begin(9600);
  Serial.println(sdInited ? "SD successful" : "SD failed");
#endif
  while (!sdInited)
    digitalWrite(LED_BUILTIN, (millis() / 250) % 2);

  harness.loadProg("init.kua");
}

//Round-robin the heartbeats
void loop () {
  bool allDead = true;
  for (uint8_t p = 0; p < machine.numProg; ++p)
    if (machine.heartbeat(p))
      allDead = false;
  //If all heartbeats have stopped, halt the Arduino
  if (allDead)
    while (true);
}
