#pragma once

#include <cstdint>

class Pixel_Grid {
 public:
  void setGridCellColour(uint16_t, uint16_t, uint32_t) {}
  void render() {}
};

class LCD_Panel {
 public:
  void changeCharArray(const char*) {}
  void render() {}
};
