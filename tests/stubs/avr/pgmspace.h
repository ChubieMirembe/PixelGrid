#pragma once

#include <cstdint>

#ifndef PROGMEM
#define PROGMEM
#endif

inline uint16_t pgm_read_word(const uint16_t* addr) {
  return *addr;
}
