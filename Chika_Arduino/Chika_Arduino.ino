#include <SPI.h>
#include <SD.h>
#include "ChVM.hpp"

void ChVM_Harness::print (const char* output) {
  Serial.println(output);
}

void ChVM_Harness::debugger (const char* output, bool showNum = false, uint32_t number = 0) {
  if (showNum) {
    Serial.print(output);
    Serial.print(" ");
    if (number < 16) Serial.print("0");
    Serial.println(number, HEX);
  } else {
    Serial.println(output);
  }
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
  Serial.println("^");
}

void ChVM_Harness::printItems (uint8_t* pItems, uint32_t n) {
  Serial.print("Items: ");
  for (uint8_t it = 0; it < n; ++it) {
    Item* item = (Item*)(pItems - (it * sizeof(Item)));
    Serial.print("l");
    Serial.print(item->len);
    Serial.print("t");
    Serial.print(item->type(), HEX);
    Serial.print(item->isConst() ? 'c' : ' ');
    Serial.print("; ");
  }
  Serial.println();
}

uint32_t ChVM_Harness::msNow () {
  return millis();
}

ChVM machine = ChVM();
uint8_t mem[CHIKA_SIZE];
uint8_t pNum = 0;

uint8_t ChVM_Harness::loadProg (const char* path) {
  File prog = SD.open(path);
  if (!prog) {
    Serial.println("Program not found");
    return 0;
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
  return pNum++;
}

uint8_t ChVM_Harness::unloadProg (const char* path) {
  return --pNum;
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
