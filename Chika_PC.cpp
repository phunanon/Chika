#include <cstdio>
#include <iostream>
#include <fstream>
#include <chrono>
#include "ChVM.cpp"

class PCHarness : IMachineHarness {
public:
  virtual void     print      (const char*);
  virtual void     debugger   (const char*, bool, uint32_t);
  virtual void     printMem   (uint8_t*, uint8_t);
  virtual void     printItems (uint8_t*, itemnum);
  virtual uint8_t  loadProg   (const char*);
  virtual uint8_t  unloadProg (const char*);
  virtual uint32_t msNow      ();
};

void PCHarness::print (const char* output) {
  printf("%s\n", output);
}

void PCHarness::debugger (const char* output, bool showNum = false, uint32_t number = 0) {
  if (showNum) {
    printf("%s", output);
    printf("%s", " ");
    if (number < 16) printf("0");
    printf("%X\n", number);
  } else {
    printf("%s\n", output);
  }
}

void PCHarness::printMem (uint8_t* mem, uint8_t by) {
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

void PCHarness::printItems (uint8_t* pItems, uint16_t n) {
  printf("Items: ");
  for (uint8_t it = 0; it < n; ++it) {
    Item* item = (Item*)(pItems - (it * sizeof(Item)));
    printf("l%d t%X %c; ", item->len, item->type(), item->isConst() ? 'c' : ' ');
  }
  printf("\n");
}

auto start_time = std::chrono::high_resolution_clock::now();
uint32_t PCHarness::msNow () {
  auto current_time = std::chrono::high_resolution_clock::now();
  return std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time).count();
}


Machine machine = Machine();
uint8_t mem[CHIKA_SIZE];
uint8_t pNum = 0;

uint8_t PCHarness::loadProg (const char* path) {
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
  machine.entry();
  return pNum++;
}

uint8_t PCHarness::unloadProg (const char* path) {
  return --pNum;
}

int main (int argc, char* argv[]) {
  PCHarness harness = PCHarness();
  machine.harness = (IMachineHarness*)(&harness);
  machine.mem = mem;

  if (argc == 2) harness.loadProg(argv[1]);
  else           harness.loadProg("init.kua");

  //Beat all hearts once, check if all are dead for early exit
  {
    bool allDead = true;
    for (uint8_t p = 0; p < pNum; ++p)
      if (machine.heartbeat(p))
        allDead = false;
    if (allDead) return 0;
  }

  //Round-robin the heartbeats
  while (true)
    for (uint8_t p = 0; p < pNum; ++p)
      machine.heartbeat(p);
}
