#include "Broker.hpp"
#include "ChVM.hpp"

Broker::Broker () {}

uint8_t indexOf (const char* str, char ch) {
  for (uint8_t i = 0; str[i]; ++i)
    if (str[i] == ch) return i;
  return 0;
}

bool topicsMatch (const char* sT, const char* mT) {
  for (uint8_t s = 0, m = 0; sT[s] && mT[m]; ) {
    if (sT[s] == '#') return true; //e.g. "test/#" match "test/test"
    if (sT[s] == '+') { //e.g. "test/+/test" match "test/test/test"
      ++s; //Skip +
      if (!s) return true; //e.g. "test/+" match "test/test/test"
      ++s; //Skip /
      uint8_t i = indexOf(mT + m, '/');
      if (!i) return false; //e.g. "test/+/ing" not match "test/test"
      m += i + 1; //Skip field
    } else
      if (sT[s] == mT[m]) { //e.g. "123" match "123"
        ++s;
        ++m;
      } else return false; //e.g. "123" not match "124"
    //e.g. "test" not match "test1" and vice versa
    if ((!sT[s] && mT[m]) || (sT[s] && !mT[m]))
      return false;
  }
  return true; //Perfect match
}

void Broker::publish (const char* topic, Item* iPayload, uint8_t* bPayload, ChVM* vm) {
  strilen tOffset = 0;
  for (subnum s = 0; s < numSub; ++s) {
    if (topicsMatch(&topics[tOffset], topic))
      vm->msgInvoker(subs[s].p, subs[s].f, topic, iPayload, bPayload, subs[s].provideT);
    tOffset += subs[s].tLen;
  }
}

void Broker::subscribe (const char* topic, prognum pNum, funcnum fNum, bool provideT) {
  sublen tLen = strlen(topic) + 1;
  memcpy(&topics[topicsLen], topic, tLen);
  topicsLen += tLen;
  subs[numSub++] = Sub {.p = pNum, .f = fNum, .tLen = tLen, .provideT = provideT};
}

void Broker::unsubscribe (prognum pNum, const char* topic) {
  strilen tOffset = 0;
  for (subnum s = 0; s < numSub; ++s) {
    if (subs[s].p != pNum || (topic && strcmp(topic, &topics[tOffset]))) {
      tOffset += subs[s].tLen;
      continue;
    }
    //Move subscriptions and subscription topics leftward into its space
    //  so long as its not the last - it'll just be truncated instead
    sublen tLen = subs[s].tLen;
    if (s + 1 != numSub) {
      memcpy(&subs[s], &subs[s + 1], (numSub - s - 1) * sizeof(Sub));
      memcpy(&topics[tOffset], &topics[tOffset + tLen], topicsLen - tOffset - tLen);
    }
    --numSub;
    --s;
    topicsLen -= tLen;
  }
}

void Broker::shiftCallbacks (prognum after) {
  for (subnum s = 0; s < numSub; ++s)
    if (subs[s].p > after)
      --subs[s].p;
}