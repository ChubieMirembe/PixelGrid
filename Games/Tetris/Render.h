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

  void setDigitsText(const char* s) {
    // exactly 6 chars on the LCD panel
    char out[6] = {' ', ' ', ' ', ' ', ' ', ' '};

    if (s) {
      size_t n = strlen(s);

      // If longer than 6, take last 6 chars; if shorter, right-align
      if (n >= 6) {
        for (int i = 0; i < 6; ++i) out[i] = s[n - 6 + i];
      } else {
        int start = 6 - (int)n;
        for (size_t i = 0; i < n; ++i) out[start + (int)i] = s[i];
      }
    }

    lcdPanel->changeCharArray(out);
  }

  // --------------------------
  // NEW: HUD (Hold / divider / Next + score last 3 digits)
  // digit 0: hold icon (colored)
  // digit 1: divider (dim)
  // digit 2: next icon (colored)
  // digit 3-5: score (white)
  // --------------------------

  void setHostHud3MasksAndScore(const uint8_t masks3[3],
                              const uint32_t cols3[3],
                              uint16_t scoreLast3)
{
  if (!lcdPanel || !strip) return;

  const uint32_t off = strip->Color(0, 0, 0);
  const uint32_t scoreCol = strip->Color(220, 220, 220);

  // Digits 0-2: raw masks + per-digit colors
  for (uint8_t i = 0; i < 3; ++i) {
    lcdPanel->setDigitOnColour(i, cols3[i]);
    lcdPanel->setDigitOffColour(i, off);
    lcdPanel->setDigitSegments(i, masks3[i]);
  }

  // Digits 3-5: score (numeric)
  uint16_t last3 = (uint16_t)(scoreLast3 % 1000);
  char c3 = (char)('0' + (last3 / 100) % 10);
  char c4 = (char)('0' + (last3 / 10) % 10);
  char c5 = (char)('0' + (last3 / 1) % 10);

  for (uint8_t i = 3; i < 6; ++i) {
    lcdPanel->setDigitOnColour(i, scoreCol);
    lcdPanel->setDigitOffColour(i, off);
  }

  lcdPanel->setDigitChar(3, c3);
  lcdPanel->setDigitChar(4, c4);
  lcdPanel->setDigitChar(5, c5);
}


static inline uint8_t SEG_BY_ID(uint8_t segId) {
  // segId 1..7 -> bit 0..6
  if (segId < 1 || segId > 7) return 0;
  return (uint8_t)(1u << (segId - 1));
}
/*
    2
1      3
    7
6      4
    5
*/
// Piece types in your game:
// 0 I(line), 1 O(square), 2 T, 3 S, 4 Z, 5 J, 6 L
static inline uint8_t pieceToMask(uint8_t type) {
  switch (type) {
    case 0: // Line = 1,6
      return SEG_BY_ID(1) | SEG_BY_ID(6);

    case 1: // Square = 6,7,4,5
      return SEG_BY_ID(6) | SEG_BY_ID(7) | SEG_BY_ID(4) | SEG_BY_ID(5);

    case 2: // T = 1,7,6
      return SEG_BY_ID(1) | SEG_BY_ID(7) | SEG_BY_ID(6);

    case 3: // S = 1,7,4,
      return SEG_BY_ID(1) | SEG_BY_ID(7) | SEG_BY_ID(4);

    case 4: // Z = 3,7,6
      return SEG_BY_ID(3) | SEG_BY_ID(7) | SEG_BY_ID(6);

    case 5: // J = 3,4,5
      return SEG_BY_ID(3) | SEG_BY_ID(4) | SEG_BY_ID(5);
    
    case 6: // L = 1,6,5
        return SEG_BY_ID(1) | SEG_BY_ID(6) | SEG_BY_ID(5);
    default:
      return 0;
  }
}

// Convert simple ASCII characters to 7-seg masks for the LCD panel.
// Add more cases here as needed.
static inline uint8_t charToMask(char ch) {
  switch (ch) {
    case 'P': // top, upper-left, upper-right, middle => segments 1,3,6,2,7
      return SEG_BY_ID(1) | SEG_BY_ID(3) | SEG_BY_ID(6) | SEG_BY_ID(2) | SEG_BY_ID(7);
    case 'C': // top, upper-left, lower-left, bottom => segments 1,6,5
      return SEG_BY_ID(1) | SEG_BY_ID(2) | SEG_BY_ID(6) | SEG_BY_ID(5);
    case 'I':
      return SEG_BY_ID(3) | SEG_BY_ID(4);
    case 'X':
      return SEG_BY_ID(1) | SEG_BY_ID(3) | SEG_BY_ID(4) | SEG_BY_ID(6) | SEG_BY_ID(7);
    case 'E':
      return SEG_BY_ID(1) | SEG_BY_ID(2) | SEG_BY_ID(5) | SEG_BY_ID(6) | SEG_BY_ID(7);
    case 'L':
      return SEG_BY_ID(1) | SEG_BY_ID(5) | SEG_BY_ID(6);
    case 'H':
      return SEG_BY_ID(1) | SEG_BY_ID(3) | SEG_BY_ID(4) | SEG_BY_ID(6) | SEG_BY_ID(7);
    case 'O':
      return SEG_BY_ID(1) | SEG_BY_ID(2) | SEG_BY_ID(3) | SEG_BY_ID(4) | SEG_BY_ID(5) | SEG_BY_ID(6);

    default:
      return 0;
  }
}
static inline uint8_t drawCat(){
  return SEG_BY_ID(1) | SEG_BY_ID(3) | SEG_BY_ID(4) | SEG_BY_ID(5) | SEG_BY_ID(6) | SEG_BY_ID(7);
}
static inline uint32_t scoreColorSmoothByScore(Adafruit_NeoPixel& strip, uint32_t score) {
  const uint32_t HUE_CYCLE_K = 6; // rainbow every 6000 points (tune)

  uint32_t k = score / 1000;
  uint16_t frac = (uint16_t)((score % 1000) * 65535UL / 1000UL); // 0..65535

  uint16_t hue0 = (uint16_t)((k * 65535UL) / HUE_CYCLE_K);
  uint16_t hue1 = (uint16_t)(((k + 1) * 65535UL) / HUE_CYCLE_K);

  int32_t dh = (int32_t)(int16_t)(hue1 - hue0); // signed 16-bit delta
  // choose shortest path around the circle
  if (dh > 32767) dh -= 65536;
  if (dh < -32768) dh += 65536;

  uint16_t hue = (uint16_t)(hue0 + (int32_t)((dh * (int32_t)frac) >> 16));

  // Slightly higher V helps visibility; tune V=150..255
  return strip.gamma32(strip.ColorHSV(hue, 255, 200));
}

  void setHudHoldNextScore(int8_t holdType, uint8_t nextType, const uint32_t pieceColors[7], uint32_t score) {
    if (!lcdPanel || !strip) return;

    const uint32_t off = strip->Color(0, 0, 0);
    const uint32_t dividerCol = strip->Color(60, 60, 60);
    const uint32_t scoreCol = strip->Color(220, 220, 220);

    // Digit 0: HOLD
    if (holdType >= 0 && holdType < 7) {
      lcdPanel->setDigitOnColour(0, pieceColors[holdType]);
      lcdPanel->setDigitOffColour(0, off);
      lcdPanel->setDigitSegments(0, pieceToMask((uint8_t)holdType));
    } else {
      // no hold yet: blank
      lcdPanel->setDigitOnColour(0, off);
      lcdPanel->setDigitOffColour(0, off);
      lcdPanel->setDigitSegments(0, 0);
    }

    // Digit 1: divider (middle bar only)
    lcdPanel->setDigitOnColour(1, dividerCol);
    lcdPanel->setDigitOffColour(1, off);
    lcdPanel->setDigitSegments(1, SEG_BY_ID(7));
    
    // Digit 2: NEXT
    lcdPanel->setDigitOnColour(2, pieceColors[nextType]);
    lcdPanel->setDigitOffColour(2, off);
    lcdPanel->setDigitSegments(2, pieceToMask(nextType));

    // Digits 3-5: score last 3 digits using chars
    uint16_t last3 = (uint16_t)(score % 1000);
    char out[6] = {' ', ' ', ' ', '0', '0', '0'};
    out[3] = (char)('0' + (last3 / 100) % 10);
    out[4] = (char)('0' + (last3 / 10) % 10);
    out[5] = (char)('0' + (last3 / 1) % 10);

    uint32_t scoreColEffective = scoreColorSmoothByScore(*strip, score); // or ...FromThousands

    for (uint8_t i = 3; i < 6; ++i) {
      lcdPanel->setDigitOnColour(i, scoreColEffective);
      lcdPanel->setDigitOffColour(i, off);
    }

    // This writes all 6 chars, but first 3 are spaces so it won't fight our segment-masks
    lcdPanel->setDigitChar(3, out[3]);
    lcdPanel->setDigitChar(4, out[4]);
    lcdPanel->setDigitChar(5, out[5]);
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

  int16_t computeTextPixelWidth(const char* s) {
    const int glyphW = 5; // pixels per glyph
    const int gap = 1;    // gap after each glyph
    const int spaceGap = 1; // extra gap for 'words' (reduced to tighten spacing)

    if (!s) return 0;
    int16_t w = 0;
    for (const char* p = s; *p; ++p) {
      if (*p == ' ') { w += spaceGap; continue; }
      // glyph
      w += glyphW;
      // gap after glyph
      w += gap;
    }
    if (w > 0) w -= gap; // remove trailing gap
    return w;
  }
  void drawTitleScroll_PIXELCATS(int16_t baseX){
    clearAllToBackground();

    int16_t y0 = (int16_t)((PLAY_H - 7) / 2);
    
      static const uint8_t P_[7] = {
      0b11110,
      0b10001, 
      0b10001, 
      0b11110, 
      0b10000, 
      0b10000, 
      0b10000
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
    static const uint8_t X_[7] = {
      0b10001, 
      0b01010, 
      0b00100, 
      0b00100, 
      0b00100, 
      0b01010, 
      0b10001
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
    static const uint8_t L_[7] = {
      0b10000, 
      0b10000, 
      0b10000, 
      0b10000, 
      0b10000, 
      0b10000, 
      0b11111
    };

    static const uint8_t C_[7] = {
      0b01110,
      0b10001,
      0b10000,
      0b10000,
      0b10000,
      0b10001,
      0b01110
    };
    static const uint8_t A_[7] = {
      0b01110,
      0b10001,
      0b10001,
      0b11111,
      0b10001,
      0b10001,
      0b10001
    };
    static const uint8_t T_[7] = {
      0b11111,
      0b00100,
      0b00100,
      0b00100,
      0b00100,
      0b00100,
      0b00100
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
    uint32_t pink = strip ? strip->Color(255, 105, 180) : TEXT_COLOR;
    int16_t x = baseX;

    // PIXEL in pink
    drawChar5x7(x,      y0, P_, pink); x += 6;
    drawChar5x7(x,      y0, I_, pink); x += 6;
    drawChar5x7(x,      y0, X_, pink); x += 6;
    drawChar5x7(x,      y0, E_, pink); x += 6;
    drawChar5x7(x,      y0, L_, pink); x += 6;

    x += 1;

    // CATS in pink
    drawChar5x7(x,      y0, C_, pink); x += 6;
    drawChar5x7(x,      y0, A_, pink); x += 6;
    drawChar5x7(x,      y0, T_, pink); x += 6;
    drawChar5x7(x,      y0, S_, pink); x += 6;

    show();
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

    

    // word:  T E T R I S
    // each letter 5px + 1px gap => 6px advance; use a small 1px word gap
    int16_t x = baseX;


    

    // TETRIS in TEXT_COLOR
    drawChar5x7(x,      y0, T_, TEXT_COLOR); x += 6;
    drawChar5x7(x,      y0, E_, TEXT_COLOR); x += 6;
    drawChar5x7(x,      y0, T_, TEXT_COLOR); x += 6;
    drawChar5x7(x,      y0, R_, TEXT_COLOR); x += 6;
    drawChar5x7(x,      y0, I_, TEXT_COLOR); x += 6;
    drawChar5x7(x,      y0, S_, TEXT_COLOR);

    show();
  }

  void drawGameOverScroll_TRYAGAIN(int16_t baseX) {
    // Fill the whole display with the game-over red background
    fillAll(GAMEOVER_RED);

    // Center the 7px tall text in the play area
    int16_t y0 = (int16_t)((PLAY_H - 7) / 2);

    // 5x7 glyphs used for "TRY AGAIN"
    static const uint8_t T_[7] = {
      0b11111,
      0b00100,
      0b00100,
      0b00100,
      0b00100,
      0b00100,
      0b00100
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
    static const uint8_t Y_[7] = {
      0b10001,
      0b01010,
      0b00100,
      0b00100,
      0b00100,
      0b00100,
      0b00100
    };
    static const uint8_t A_[7] = {
      0b01110,
      0b10001,
      0b10001,
      0b11111,
      0b10001,
      0b10001,
      0b10001
    };
    static const uint8_t G_[7] = {
      0b01110,
      0b10001,
      0b10000,
      0b10111,
      0b10001,
      0b10001,
      0b01110
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
    static const uint8_t N_[7] = {
      0b10001,
      0b11001,
      0b10101,
      0b10011,
      0b10001,
      0b10001,
      0b10001
    };

    uint32_t col = strip ? strip->Color(255, 255, 255) : TEXT_COLOR;
    int16_t x = baseX;

    // TRY
    drawChar5x7(x,      y0, T_, col); x += 6;
    drawChar5x7(x,      y0, R_, col); x += 6;
    drawChar5x7(x,      y0, Y_, col); x += 6;

    // small space between words
    x += 1;

    // AGAIN
    drawChar5x7(x,      y0, A_, col); x += 6;
    drawChar5x7(x,      y0, G_, col); x += 6;
    drawChar5x7(x,      y0, A_, col); x += 6;
    drawChar5x7(x,      y0, I_, col); x += 6;
    drawChar5x7(x,      y0, N_, col); x += 6;

    show();
  }

  void drawGameOverHold() {
    fillAll(GAMEOVER_RED);
    show();
  }
};