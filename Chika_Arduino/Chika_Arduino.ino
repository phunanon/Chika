#include <SPI.h>
#include <SD.h>
#include "Machine.hpp"

class ArduinoHarness : IMachineHarness {
public:
  virtual void     print      (const char*);
  virtual void     debugger   (const char*, bool, uint32_t);
  virtual void     printMem   (uint8_t*, uint8_t);
  virtual void     printItems (uint8_t*, itemnum);
  virtual uint8_t  loadProg   (const char*);
  virtual uint8_t  unloadProg (const char*);
  virtual uint32_t msNow      ();
};

void ArduinoHarness::print (const char* output) {
  Serial.println(output);
}

void ArduinoHarness::debugger (const char* output, bool showNum = false, uint32_t number = 0) {
  if (showNum) {
    Serial.print(output);
    Serial.print(" ");
    if (number < 16) Serial.print("0");
    Serial.println(number, HEX);
  } else {
    Serial.println(output);
  }
}

void ArduinoHarness::printMem (uint8_t* mem, uint8_t by) {
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

void ArduinoHarness::printItems (uint8_t* pItems, uint16_t n) {
  Serial.print("Items: ");
  for (uint8_t it = 0; it < n; ++it) {
    Item* item = (Item*)(pItems - ((it+1) * sizeof(Item)));
    Serial.print("l");
    Serial.print(item->len);
    Serial.print("t");
    Serial.print(item->type());
    Serial.print(" ");
    Serial.print(item->isConst() ? 'c' : ' ');
    Serial.print("; ");
  }
  printf("\n");
}

uint32_t ArduinoHarness::msNow () {
  return millis();
}

Machine machine = Machine();
uint8_t mem[CHIKA_SIZE];
uint8_t pNum = 0;

uint8_t ArduinoHarness::loadProg (const char* path) {
  File prog = SD.open(path);
  if (!prog) {
    Serial.println("Program not found");
    return 0;
  }
  machine.setPNum(pNum);
  uint8_t* rom = machine.pROM;
  bytenum ramLen;
  for (uint8_t b = 0; b < sizeof(ramLen); ++b)
    *(char*)&ramLen = prog.read();
  progs[pNum].ramLen = ramLen <= MAX_PROG_RAM ? ramLen : MAX_PROG_RAM;
  uint16_t pByte = 0;
  while (prog.available())
    rom[pByte++] = prog.read();
  machine.romLen(pByte);
  prog.close();
  return pNum++;
}

uint8_t ArduinoHarness::unloadProg (const char* path) {
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
  
  ArduinoHarness harness = ArduinoHarness();
  machine.mem = mem;
  machine.harness = (IMachineHarness*)(&harness);

  harness.loadProg("init.kua");
}

void loop () {
  //Round-robin the heartbeats
  for (uint8_t p = 0; p < pNum; ++p)
    machine.heartbeat(p);
  delay(4000);
}
