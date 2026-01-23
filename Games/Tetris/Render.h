#pragma once
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <PixelGridCore.h>
#include "Pins.h"

static inline uint16_t playRowToPixelRow(uint8_t logicalRow) {
  return (uint16_t)(MATRIX_ROWS - 1 - (PREVIEW_ROWS + logicalRow));
}
static inline uint16_t previewRowToPixelRow(uint8_t p) {
  return (uint16_t)(MATRIX_ROWS - 1 - p);
}

struct Renderer {
  Adafruit_NeoPixel* strip = nullptr;
  Pixel_Grid* pixelGrid = nullptr;
  LCD_Panel* lcdPanel = nullptr;

  // colours
  uint32_t PREVIEW_BG = 0;
  uint32_t PLAY_BG = 0;
  uint32_t GAMEOVER_RED = 0;
  uint32_t TEXT_COLOR = 0;
  uint32_t GHOST_COLOR = 0;

  void begin(Adafruit_NeoPixel* s, Pixel_Grid* g, LCD_Panel* l) {
    strip = s; pixelGrid = g; lcdPanel = l;

    PREVIEW_BG   = strip->Color(80, 80, 120);
    PLAY_BG      = strip->Color(6, 6, 12);
    GAMEOVER_RED = strip->Color(30, 0, 0);
    TEXT_COLOR   = strip->Color(220, 220, 220);

    // IMPORTANT: set ghost colour here so it's always available
    GHOST_COLOR  = strip->Color(120, 120, 120);
  }

  void setScoreDigits(uint32_t score) {
    char tmp[7];
    char out[6];
    snprintf(tmp, sizeof(tmp), "%6lu", (unsigned long)score);
    for (uint8_t i = 0; i < 6; ++i) out[i] = tmp[i];
    lcdPanel->changeCharArray(out);
  }

  void clearAllToBackground() {
    // preview rows
    for (uint8_t p = 0; p < PREVIEW_ROWS; ++p) {
      uint16_t r = previewRowToPixelRow(p);
      for (uint8_t x = 0; x < W; ++x) pixelGrid->setGridCellColour(r, x, PREVIEW_BG);
    }
    // play area
    for (uint8_t y = 0; y < PLAY_H; ++y) {
      uint16_t r = playRowToPixelRow(y);
      for (uint8_t x = 0; x < W; ++x) pixelGrid->setGridCellColour(r, x, PLAY_BG);
    }
  }

  void fillAll(uint32_t c) {
    for (uint8_t y = 0; y < MATRIX_ROWS; ++y) {
      for (uint8_t x = 0; x < W; ++x) pixelGrid->setGridCellColour((uint16_t)y, x, c);
    }
  }

  void show() {
    lcdPanel->render();
    pixelGrid->render();
    strip->show();
  }

  // ===== Title text drawing (5x7 font, right-to-left scrolling) =====
  // Coordinates: (0,0) is top-left of play area (not preview), y in [0..PLAY_H-1]
  void drawChar5x7(int16_t x0, int16_t y0, const uint8_t glyph[7], uint32_t c) {
    for (int y = 0; y < 7; ++y) {
      uint8_t rowBits = glyph[y];
      for (int x = 0; x < 5; ++x) {
        bool on = (rowBits >> (4 - x)) & 1;
        if (!on) continue;
        int16_t gx = x0 + x;
        int16_t gy = y0 + y;
        if (gx < 0 || gx >= (int16_t)W) continue;
        if (gy < 0 || gy >= (int16_t)PLAY_H) continue;
        pixelGrid->setGridCellColour(playRowToPixelRow((uint8_t)gy), (uint16_t)gx, c);
      }
    }
  }

  void drawTitleScroll_TETRIS(int16_t baseX) {
    clearAllToBackground();

    // Center the 7px tall text in the play area
    int16_t y0 = (int16_t)((PLAY_H - 7) / 2);

    // Simple 5x7 glyphs (bits per row, 5 columns)
    static const uint8_t T_[7] = {
      0b11111,
      0b00100,
      0b00100,
      0b00100,
      0b00100,
      0b00100,
      0b00100
    };
    static const uint8_t E_[7] = {
      0b11111,
      0b10000,
      0b10000,
      0b11110,
      0b10000,
      0b10000,
      0b11111
    };
    static const uint8_t R_[7] = {
      0b11110,
      0b10001,
      0b10001,
      0b11110,
      0b10100,
      0b10010,
      0b10001
    };
    static const uint8_t I_[7] = {
      0b11111,
      0b00100,
      0b00100,
      0b00100,
      0b00100,
      0b00100,
      0b11111
    };
    static const uint8_t S_[7] = {
      0b01111,
      0b10000,
      0b10000,
      0b01110,
      0b00001,
      0b00001,
      0b11110
    };

    // word: T E T R I S  (6 letters)
    // each letter 5px + 1px gap => 6px advance
    int16_t x = baseX;

    drawChar5x7(x,      y0, T_, TEXT_COLOR); x += 6;
    drawChar5x7(x,      y0, E_, TEXT_COLOR); x += 6;
    drawChar5x7(x,      y0, T_, TEXT_COLOR); x += 6;
    drawChar5x7(x,      y0, R_, TEXT_COLOR); x += 6;
    drawChar5x7(x,      y0, I_, TEXT_COLOR); x += 6;
    drawChar5x7(x,      y0, S_, TEXT_COLOR);

    show();
  }

  void drawGameOverHold() {
    fillAll(GAMEOVER_RED);
    show();
  }
};
