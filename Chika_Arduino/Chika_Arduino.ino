#include <SPI.h>
#include <SD.h>
#include "ChVM.hpp"

void ChVM_Harness::print (const char* output) {
  Serial.println(output);
}

void ChVM_Harness::printInt (const char* output, uint32_t number) {
  Serial.print(output);
  Serial.print(" 0x");
  Serial.print(number, HEX);
  Serial.print(" ; ");
  Serial.println(number);
}

void ChVM_Harness::printMem (uint8_t* mem, uint8_t by) {
  uint8_t* mEnd = mem + by;
  for (uint8_t* m = mem - by; m < mEnd; ++m) {
    if (*m < 16) Serial.print("0");
    Serial.print(*m, HEX);
  }
  Serial.println();
  for (uint8_t i = 0; i < by; ++i)
    Serial.print("  ");
  Serial.println("^^");
}

void ChVM_Harness::printItems (uint8_t* pItems, uint32_t n) {
  Serial.print("Items: ");
  for (uint8_t it = 0; it < n; ++it) {
    Item* item = (Item*)(pItems - (it * sizeof(Item)));
    Serial.print(it);
    Serial.print("#l");
    Serial.print(item->len);
    Serial.print("t");
    Serial.print(item->type(), HEX);
    Serial.print(item->isConst() ? 'c' : ' ');
    Serial.print(" ");
  }
  Serial.println();
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

ChVM machine = ChVM();
uint8_t mem[CHIKA_SIZE];
uint8_t pNum = 0;

bool ChVM_Harness::loadProg (const char* path) {
  File prog = SD.open(path);
  if (!prog) {
    Serial.println("Program not found");
    return false;
  }
  bytenum ramLen;
  {
    char ramRequest[4];
    for (uint8_t b = 0; b < sizeof(ramLen); ++b)
      ramRequest[b] = prog.read();
    memcpy(&ramLen, ramRequest, sizeof(ramLen));
  }
  progs[pNum].ramLen = ramLen <= MAX_PROG_RAM ? ramLen : MAX_PROG_RAM;
  machine.setPNum(pNum);
  uint16_t pByte = 0;
  while (prog.available())
    machine.pROM[pByte++] = prog.read();
  prog.close();
  machine.romLen(pByte);
  machine.entry();
  return ++pNum;
}

void ChVM_Harness::unloadProg (const char* path) {
  --pNum;
}

void setup() {
  while (!Serial);
  Serial.begin(9600);
  
  Serial.print("Initialising SD card... ");
  if (!SD.begin(SDCARD_SS_PIN)) {
    Serial.println("failed.");
    while (1);
  }
  Serial.println("successful.");

  ChVM_Harness harness = ChVM_Harness();
  machine.mem = mem;
  machine.harness = &harness;

  harness.loadProg("init.kua");
}

void loop () {
  //Beat all hearts once, check if all are dead for early exit
  {
    bool allDead = true;
    for (uint8_t p = 0; p < pNum; ++p)
      if (machine.heartbeat(p))
        allDead = false;
    if (allDead)
      while (true);
  }

  //Round-robin the heartbeats
  while (true)
    for (uint8_t p = 0; p < pNum; ++p)
      machine.heartbeat(p);
}
