#pragma once

#include <cstdint>

class Adafruit_NeoPixel {
 public:
  uint32_t Color(uint8_t r, uint8_t g, uint8_t b) {
    return (static_cast<uint32_t>(r) << 16)
         | (static_cast<uint32_t>(g) << 8)
         | static_cast<uint32_t>(b);
  }

  void show() {}
};
