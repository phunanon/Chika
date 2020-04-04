#ifndef ARDUINO

#include <cstdio>
#include <chrono>
#include "ChVM.hpp"

void ChVM_Harness::print (const char* output) {
  printf("%s", output);
  fflush(stdout);
}

void ChVM_Harness::printInt (const char* output, uint32_t number) {
  printf("%s 0x%X %d\n", output, number, number);
}

void ChVM_Harness::printMem (uint8_t* mem, uint8_t by) {
  uint8_t left = by / 2;
  for (uint8_t i = 0; i < left; ++i)
    printf("%X ", (uint32_t)(intptr_t)((mem - left) + i) % 16);
  printf(". ");
  for (uint8_t i = 1; i < by; ++i)
    printf("%01X ", (uint32_t)(intptr_t)(mem + i) % 16);
  printf("\n");
  uint8_t* mEnd = mem + by;
  for (uint8_t* m = mem - left; m < mEnd; ++m) {
    if (*m < 16) printf("0");
    printf("%X", *m);
  }
  printf("\n");
}

void ChVM_Harness::printItems (uint8_t* pItems, uint32_t n) {
  printf("Items: ");
  for (uint8_t it = 0; it < n; ++it) {
    Item* item = (Item*)(pItems - (it * sizeof(Item)));
    printf("%d.%d.%X ", it, item->len, item->type);
  }
  printf("\n");
}


void ChVM_Harness::pinMod (uint8_t pin, bool mode) {
  printf("%d%s ", pin, mode ? "O" : "I");
  fflush(stdout);
}
bool ChVM_Harness::digIn  (uint8_t pin) {
  return false;
}
void ChVM_Harness::digOut (uint8_t pin, bool val) {
  printf("%d%s ", pin, val ? "H" : "L");
  fflush(stdout);
}
uint16_t ChVM_Harness::anaIn  (uint8_t pin) {
  return 0;
}
void ChVM_Harness::anaOut (uint8_t pin, uint16_t val) {
  printf("%d:%d ", pin, val);
  fflush(stdout);
}


int fsize (FILE *fp) {
    int prev = ftell(fp);
    fseek(fp, 0L, SEEK_END);
    int sz = ftell(fp);
    fseek(fp, prev, SEEK_SET);
    return sz;
}

//Reads from `offset` in the file for `count` bytes
//If `count` is 0 the rest of the file is read
int32_t ChVM_Harness::fileRead (const char* path, uint8_t* blob, uint32_t offset, uint32_t count) {
  FILE* fp = fopen(path, "rb");
  if (!fp) return 0;
  size_t fLen = fsize(fp);
  if (offset + count > fLen) {
    if (offset > fLen) return 0;
    count = 0;
  }
  if (!count) count = fLen - offset;
  fseek(fp, offset, SEEK_SET);
  count = fread(blob, sizeof(uint8_t), count, fp);
  fclose(fp);
  return count;
}

bool ChVM_Harness::fileWrite (const char* path, uint8_t* blob, uint32_t offset, uint32_t count) {
  FILE* fp = fopen(path, "r+b");
  if (!fp) return false;
  fseek(fp, offset, SEEK_SET);
  fwrite(blob, sizeof(uint8_t), count, fp);
  fclose(fp);
  return true;
}

bool ChVM_Harness::fileAppend (const char* path, uint8_t* blob, uint32_t count) {
  FILE* fp = fopen(path, "ab");
  if (!fp) return false;
  fwrite(blob, sizeof(uint8_t), count, fp);
  fclose(fp);
  return true;
}

bool ChVM_Harness::fileDelete (const char* path) {
  return remove(path);
}

int32_t ChVM_Harness::fileSize (const char* path) {
  FILE* fp = fopen(path, "rb");
  int32_t fSize = fp ? fsize(fp) : -1;
  if (fp) fclose(fp);
  return fSize;
}


auto start_time = std::chrono::high_resolution_clock::now();
uint32_t ChVM_Harness::msNow () {
  auto current_time = std::chrono::high_resolution_clock::now();
  return std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time).count();
}


ChVM_Harness harness = ChVM_Harness();
ChVM machine = ChVM(&harness);

bool ChVM_Harness::loadProg (const char* path) {
  FILE* fp = fopen(path, "rb");
  if (!fp) return false;
  size_t fLen = fsize(fp);
  bytenum memLen;
  fread((char*)&memLen, sizeof(proglen), 1, fp);
  machine.switchToProg(machine.numProg++, fLen - sizeof(proglen), memLen);
  fread((char*)machine.getPROM(), sizeof(proglen), fLen, fp);
  fclose(fp);
  machine.entry();
  return true;
}

int main (int argc, char* argv[]) {
  if (argc == 2) harness.loadProg(argv[1]);
  else           harness.loadProg("init.kua");
  
  //Keep the machine's heart beating until dead
  while (machine.heartbeat());
}

#endif