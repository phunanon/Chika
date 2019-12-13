#include <cstdio>
#include <iostream>
#include <fstream>
#include "Machine.cpp"

void debugger (const char* output, bool showNum = false, uint32_t number = 0) {
  if (showNum) {
    printf("%s", output);
    printf("%s", " ");
    if (number < 16) printf("0");
    printf("%X\n", number);
  } else {
    printf("%s\n", output);
  }
}

void printMem (uint8_t* mem, uint8_t by) {
  uint8_t margin = by * .5;
  uint8_t left = by - margin;
  uint8_t right = by + margin;
  for (uint8_t i = 0; i < left; ++i)
    printf("  ");
  printf("v\n");
  uint8_t* mEnd = mem + right;
  for (uint8_t* m = mem - left; m < mEnd; ++m) {
    if (*m < 16) printf("0");
    printf("%X", *m);
  }
  printf("\n");
}

void printItems (uint8_t* pItems, uint16_t n) {
  printf("Items: ");
  for (uint8_t it = 0; it < n; ++it) {
    Item* item = (Item*)(pItems - ((it+1) * sizeof(Item)));
    printf("l%d t%X %c; ", item->len, item->type(), item->isConst() ? 'c' : ' ');
  }
  printf("\n");
}

void delay (long unsigned int t) {}


Machine machine = Machine();
uint8_t mem[CHIKA_SIZE];
memolen progSize;
uint8_t pNum = 0;

uint8_t loadProg (const char* path) {
  machine.pNum = pNum;
  std::ifstream fl(path);
  fl.seekg( 0, std::ios::end);
  size_t len = fl.tellg();
  fl.seekg(0, std::ios::beg);
  fl.read((char*)machine.pROM(), len);
  fl.close();
  machine.romLen(len);
  return pNum++;
}

void loop () {
  //Round-robin the heartbeats
  for (uint8_t p = 0; p < pNum; ++p)
    machine.heartbeat(p);
}

int main (int argc,  char* argv[]) {
  //FIXME currently only one file at a time: Read config file
  progSize = CHIKA_SIZE;

  machine.mem = mem;
  machine.progSize = progSize;
  machine.loadProg = loadProg;
  machine.delay = delay;
  machine.debugger = debugger;
  machine.printMem = printMem;
  machine.printItems = printItems;

  machine.loadProg(argv[1]);

  loop();
}
