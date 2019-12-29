#include <cstdio>
#include <iostream>
#include <fstream>
#include <chrono>
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
    Item* item = (Item*)(pItems - (it * sizeof(Item)));
    printf("l%d t%X %c; ", item->len, item->type(), item->isConst() ? 'c' : ' ');
  }
  printf("\n");
}

auto start_time = std::chrono::high_resolution_clock::now();
uint32_t msNow () {
  auto current_time = std::chrono::high_resolution_clock::now();
  return std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time).count();
}


Machine machine = Machine();
uint8_t mem[CHIKA_SIZE];
uint8_t pNum = 0;

uint8_t loadProg (const char* path) {
  std::ifstream fl(path, std::ios::in | std::ios::binary);
  fl.seekg(0, std::ios::end);
  size_t fLen = fl.tellg();
  fl.seekg(0, std::ios::beg);
  bytenum ramLen;
  fl.read((char*)&ramLen, sizeof(bytenum));
  progs[pNum].ramLen = ramLen <= MAX_PROG_RAM ? ramLen : MAX_PROG_RAM;
  machine.setPNum(pNum);
  fl.read((char*)machine.pROM, fLen - sizeof(bytenum));
  fl.close();
  machine.romLen(fLen - sizeof(bytenum));
  return pNum++;
}

void loop () {
  //Round-robin the heartbeats
  for (uint8_t p = 0; p < pNum; ++p)
    machine.heartbeat(p);
}

int main (int argc,  char* argv[]) {
  machine.mem = mem;
  machine.loadProg = loadProg;
  machine.msNow = msNow;
  machine.debugger = debugger;
  machine.printMem = printMem;
  machine.printItems = printItems;

  if (argc == 2)
    machine.loadProg(argv[1]);
  else
    machine.loadProg("init.kua");

  while (true)
    loop();
}
