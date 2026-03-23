#pragma once

#include <cstdint>
#include <cstdlib>

#ifndef INPUT_PULLUP
#define INPUT_PULLUP 0x2
#endif

#ifndef LOW
#define LOW 0x0
#endif

inline void pinMode(uint8_t, uint8_t) {}
inline int digitalRead(uint8_t) { return LOW; }

inline uint32_t& fakeMillisRef() {
  static uint32_t fakeMillis = 0;
  return fakeMillis;
}

inline uint32_t millis() {
  return fakeMillisRef();
}

inline void setMillis(uint32_t value) {
  fakeMillisRef() = value;
}

inline long random(long max) {
  return (max > 0) ? 0 : 0;
}

inline long random(long min, long max) {
  (void)max;
  return min;
}

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif
