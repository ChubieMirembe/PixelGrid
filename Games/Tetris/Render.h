// Render.h
#pragma once
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <PixelGridCore.h>
#include "Pins.h"
#include "Game.h"

// These are created in the .ino
extern Adafruit_NeoPixel strip;
extern Pixel_Grid* pixelGrid;
extern LCD_Panel* lcdPanel;

// Colours live here (simple: one place to tweak visuals)
static uint32_t PREVIEW_BG_COLOR_U32;
static uint32_t PLAY_BG_COLOR_U32;
static uint32_t GHOST_COLOR_U32;
static const uint8_t GHOST_PERCENT = 36;

static uint32_t PIECE_COLORS_U32[7];

static inline uint16_t playRowToPixelRow(uint8_t logicalRow) {
  return (uint16_t)(MATRIX_ROWS - 1 - (PREVIEW_ROWS + logicalRow));
}
static inline uint16_t previewRowToPixelRow(uint8_t p) {
  return (uint16_t)(MATRIX_ROWS - 1 - p);
}

static inline uint8_t colR(uint32_t c) { return (uint8_t)((c >> 16) & 0xFF); }
static inline uint8_t colG(uint32_t c) { return (uint8_t)((c >> 8) & 0xFF); }
static inline uint8_t colB(uint32_t c) { return (uint8_t)(c & 0xFF); }

static inline uint32_t dimColor(uint32_t color, uint8_t percent) {
  uint8_t r = (uint8_t)((uint16_t)colR(color) * percent / 100);
  uint8_t g = (uint8_t)((uint16_t)colG(color) * percent / 100);
  uint8_t b = (uint8_t)((uint16_t)colB(color) * percent / 100);
  return strip.Color(r, g, b);
}

// ---------- Background + previews ----------
static inline void drawBackgroundRows() {
  for (uint8_t p = 0; p < PREVIEW_ROWS; ++p) {
    uint16_t pixelRow = previewRowToPixelRow(p);
    for (uint8_t col = 0; col < W; ++col)
      pixelGrid->setGridCellColour(pixelRow, col, PREVIEW_BG_COLOR_U32);
  }

  for (uint8_t pr = 0; pr < PLAY_H; ++pr) {
    uint16_t pixelRow = playRowToPixelRow(pr);
    for (uint8_t col = 0; col < W; ++col)
      pixelGrid->setGridCellColour(pixelRow, col, PLAY_BG_COLOR_U32);
  }
}

static inline void drawPreviews() {
  // HOLD preview (left side of preview rows)
  if (holdType >= 0) {
    uint16_t holdMask = shapeMask((uint8_t)holdType, 0);
    for (uint8_t py = 0; py < 4; ++py) {
      for (uint8_t px = 0; px < 4; ++px) {
        if (!maskCell(holdMask, px, py)) continue;
        int pr = (int)py, pc = (int)px;
        if (pr >= 0 && pr < PREVIEW_ROWS && pc >= 0 && pc < W) {
          pixelGrid->setGridCellColour(previewRowToPixelRow((uint8_t)pr), (uint16_t)pc, PIECE_COLORS_U32[holdType]);
        }
      }
    }
  }

  // NEXT preview (right side of preview rows)
  uint16_t nextMask = shapeMask((uint8_t)nextPiece.type, 0);
  for (uint8_t py = 0; py < 4; ++py) {
    for (uint8_t px = 0; px < 4; ++px) {
      if (!maskCell(nextMask, px, py)) continue;
      int pr = (int)py, pc = 6 + (int)px;
      if (pr >= 0 && pr < PREVIEW_ROWS && pc >= 0 && pc < W) {
        pixelGrid->setGridCellColour(previewRowToPixelRow((uint8_t)pr), (uint16_t)pc, PIECE_COLORS_U32[nextPiece.type]);
      }
    }
  }
}

// ---------- Locked blocks ----------
static inline void drawLockedBlocks_part1() {
  uint8_t mid = PLAY_H / 2;
  for (uint8_t y = 0; y < mid; ++y) {
    uint16_t pixelRow = playRowToPixelRow(y);
    for (uint8_t x = 0; x < W; ++x) {
      uint8_t v = board[y][x];
      if (v == 0) continue;
      pixelGrid->setGridCellColour(pixelRow, x, PIECE_COLORS_U32[v - 1]);
    }
  }
}
static inline void drawLockedBlocks_part2() {
  uint8_t mid = PLAY_H / 2;
  for (uint8_t y = mid; y < PLAY_H; ++y) {
    uint16_t pixelRow = playRowToPixelRow(y);
    for (uint8_t x = 0; x < W; ++x) {
      uint8_t v = board[y][x];
      if (v == 0) continue;
      pixelGrid->setGridCellColour(pixelRow, x, PIECE_COLORS_U32[v - 1]);
    }
  }
}
static inline void drawLockedBlocks() { drawLockedBlocks_part1(); drawLockedBlocks_part2(); }

// ---------- Ghost piece ----------
static inline int8_t computeGhostY() {
  int8_t gy = curY;
  while (validAt(curX, gy + 1, curPiece.rot)) gy++;
  return gy;
}

static inline void drawGhostPiece() {
  int8_t gy = computeGhostY();
  if (gy < 0) return;

  uint16_t mask = shapeMask((uint8_t)curPiece.type, curPiece.rot);
  uint32_t ghostColor = dimColor(GHOST_COLOR_U32, GHOST_PERCENT);

  for (uint8_t cy = 0; cy < 4; ++cy) {
    for (uint8_t cx = 0; cx < 4; ++cx) {
      if (!maskCell(mask, cx, cy)) continue;

      int8_t bx = curX + (int8_t)cx;
      int8_t by = gy + (int8_t)cy;
      if (by < 0 || bx < 0 || bx >= (int8_t)W || by >= (int8_t)PLAY_H) continue;

      // outline-ish: draw only edge cells
      bool draw = false;
      const int8_t nx[4] = { -1, 1, 0, 0 };
      const int8_t ny[4] = { 0, 0, -1, 1 };
      for (uint8_t n = 0; n < 4; ++n) {
        int8_t mx = (int8_t)cx + nx[n], my = (int8_t)cy + ny[n];
        if (mx < 0 || mx >= 4 || my < 0 || my >= 4) { draw = true; break; }
        if (!maskCell(mask, (uint8_t)mx, (uint8_t)my)) { draw = true; break; }
      }
      if (!draw) continue;

      uint16_t pixelRow = playRowToPixelRow((uint8_t)by);
      pixelGrid->setGridCellColour(pixelRow, (uint16_t)bx, ghostColor);
    }
  }
}

// ---------- Current piece ----------
static inline void drawCurrentPiece() {
  uint16_t mask = shapeMask((uint8_t)curPiece.type, curPiece.rot);
  for (uint8_t cy = 0; cy < 4; ++cy) {
    for (uint8_t cx = 0; cx < 4; ++cx) {
      if (!maskCell(mask, cx, cy)) continue;

      int8_t bx = curX + (int8_t)cx;
      int8_t by = curY + (int8_t)cy;

      if (by >= 0 && bx >= 0 && bx < (int8_t)W && by < (int8_t)PLAY_H) {
        uint16_t pixelRow = playRowToPixelRow((uint8_t)by);
        pixelGrid->setGridCellColour(pixelRow, (uint16_t)bx, PIECE_COLORS_U32[curPiece.type]);
      }
    }
  }
}

static inline void finalizeDigitsAndShow() {
  lcdPanel->render();
  pixelGrid->render();
  strip.show();
}

static inline void renderFrame() {
  if (gameOver) {
    for (uint8_t p = 0; p < PREVIEW_ROWS; ++p) {
      for (uint8_t col = 0; col < W; ++col) {
        pixelGrid->setGridCellColour(previewRowToPixelRow(p), col, strip.Color(30, 0, 0));
      }
    }
    for (uint8_t pr = 0; pr < PLAY_H; ++pr) {
      for (uint8_t col = 0; col < W; ++col) {
        pixelGrid->setGridCellColour(playRowToPixelRow(pr), col, strip.Color(30, 0, 0));
      }
    }
    finalizeDigitsAndShow();
    return;
  }

  drawBackgroundRows();
  drawPreviews();
  drawLockedBlocks();
  drawGhostPiece();
  drawCurrentPiece();
  finalizeDigitsAndShow();
}

static inline void renderInitColours() {
  PREVIEW_BG_COLOR_U32 = strip.Color(80, 80, 120);
  PLAY_BG_COLOR_U32    = strip.Color(6, 6, 12);
  GHOST_COLOR_U32      = strip.Color(120, 120, 120);

  PIECE_COLORS_U32[0] = strip.Color(0, 220, 220);
  PIECE_COLORS_U32[1] = strip.Color(230, 230, 0);
  PIECE_COLORS_U32[2] = strip.Color(180, 0, 220);
  PIECE_COLORS_U32[3] = strip.Color(0, 220, 0);
  PIECE_COLORS_U32[4] = strip.Color(220, 0, 0);
  PIECE_COLORS_U32[5] = strip.Color(0, 0, 220);
  PIECE_COLORS_U32[6] = strip.Color(255, 120, 0);
}
