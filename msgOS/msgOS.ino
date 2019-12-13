#include <SPI.h>
#include <SD.h>
#include "Machine.hpp"

void debugger (const char* output, bool showNum = false, uint32_t number = 0) {
  if (showNum) {
    Serial.print(output);
    Serial.print(" ");
    if (number < 16) Serial.print("0");
    Serial.println(number, HEX);
  } else {
    Serial.println(output);
  }
}

void printMem (uint8_t* mem, uint8_t by) {
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

void printItems (uint8_t* pItems, uint16_t n) {
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

Machine machine = Machine();
uint8_t mem[CHIKA_SIZE];
memolen progSize;
uint8_t pNum = 0;

uint8_t loadProg (const char* path) {
  File prog = SD.open(path);
  if (!prog) {
    Serial.println("Program not found");
return 0;
  }
  uint16_t pByte = 0;
  machine.pNum = pNum;
  uint8_t* rom = machine.pROM();
  while (prog.available())
    rom[pByte++] = prog.read();
  machine.romLen(pByte);
  prog.close();
  return pNum++;
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
  
  //Read config file
  File setF = SD.open("msgOS.set");
  if (setF) {
    progSize = CHIKA_SIZE / setF.read();
    setF.close();
  } else {
    Serial.println("Using defaults.");
    progSize = CHIKA_SIZE;
  }
  
  machine.mem = mem;
  machine.progSize = progSize;
  machine.loadProg = loadProg;
  machine.delay = delay;
  machine.debugger = debugger;
  machine.printMem = printMem;
  machine.printItems = printItems;

  machine.loadProg("init.chi");
}

void loop () {
  //Round-robin the heartbeats
  for (uint8_t p = 0; p < pNum; ++p)
    machine.heartbeat(p);
  delay(4000);
}
