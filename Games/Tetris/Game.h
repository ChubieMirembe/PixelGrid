#pragma once
#include <Arduino.h>
#include <avr/pgmspace.h>
#include "Pins.h"
#include "Input.h"
#include "Render.h"

// ===== Game tuning =====
static const uint16_t BASE_FALL_MS = 550;
static const uint8_t  LINES_PER_LEVEL = 10;
static const uint16_t FALL_DECREMENT = 40;
static const uint16_t MIN_FALL_MS = 80;

static const uint16_t SOFT_DROP_MIN_MS = 55;
static const uint16_t SOFT_DROP_DIVISOR = 4;

static const uint8_t GHOST_PERCENT = 36;

struct TetrisGame {
  // Board: 0 empty, 1..7 filled
  uint8_t board[PLAY_H][W];

  // Shapes
  static const uint16_t SHAPES[7][4] PROGMEM;

  struct Piece { int8_t type; uint8_t rot; };

  Piece curPiece;
  Piece nextPiece;

  int8_t curX = 3;
  int8_t curY = 0;

  int8_t holdType = -1;
  bool holdLocked = false;

  bool gameOver = false;
  uint32_t score = 0;

  uint32_t totalLinesCleared = 0;
  uint8_t level = 0;
  uint16_t fallDelayMs = BASE_FALL_MS;

  uint32_t tFall = 0;

  uint32_t PIECE_COLORS[7];
  uint32_t PREVIEW_BG = 0;
  uint32_t PLAY_BG = 0;
  uint32_t GHOST_COLOR = 0;

  // init colours using renderer strip
  void initColours(Renderer& r) {
    PREVIEW_BG = r.PREVIEW_BG;
    PLAY_BG    = r.PLAY_BG;
    GHOST_COLOR = r.strip->Color(120,120,120);

    PIECE_COLORS[0] = r.strip->Color(0, 220, 220);
    PIECE_COLORS[1] = r.strip->Color(230, 230, 0);
    PIECE_COLORS[2] = r.strip->Color(180, 0, 220);
    PIECE_COLORS[3] = r.strip->Color(0, 220, 0);
    PIECE_COLORS[4] = r.strip->Color(220, 0, 0);
    PIECE_COLORS[5] = r.strip->Color(0, 0, 220);
    PIECE_COLORS[6] = r.strip->Color(255, 120, 0);
  }

  static inline uint16_t shapeMask(uint8_t type, uint8_t rot) {
    return pgm_read_word(&SHAPES[type][rot & 3]);
  }

  static inline bool maskCell(uint16_t mask, uint8_t cx, uint8_t cy) {
    uint8_t bitIndex = (uint8_t)(15 - (cy * 4 + cx));
    return (mask >> bitIndex) & 1;
  }

  static inline uint32_t dimColor(Adafruit_NeoPixel& strip, uint32_t color, uint8_t percent) {
    uint8_t r = (uint8_t)(((color >> 16) & 0xFF) * (uint16_t)percent / 100);
    uint8_t g = (uint8_t)(((color >> 8) & 0xFF)  * (uint16_t)percent / 100);
    uint8_t b = (uint8_t)(( color        & 0xFF) * (uint16_t)percent / 100);
    return strip.Color(r, g, b);
  }

  void clearBoard() {
    for (uint8_t y = 0; y < PLAY_H; ++y)
      for (uint8_t x = 0; x < W; ++x)
        board[y][x] = 0;
  }

  bool validAtParams(uint8_t type, uint8_t rot, int8_t nx, int8_t ny) const {
    uint16_t mask = shapeMask(type, rot);
    for (uint8_t cy = 0; cy < 4; ++cy) {
      for (uint8_t cx = 0; cx < 4; ++cx) {
        if (!maskCell(mask, cx, cy)) continue;
        int8_t bx = nx + (int8_t)cx;
        int8_t by = ny + (int8_t)cy;

        if (bx < 0 || bx >= (int8_t)W) return false;
        if (by >= (int8_t)PLAY_H) return false;
        if (by >= 0 && board[by][bx] != 0) return false;
      }
    }
    return true;
  }

  bool validAt(int8_t nx, int8_t ny, uint8_t nrot) const {
    return validAtParams((uint8_t)curPiece.type, nrot, nx, ny);
  }

  uint8_t clearLines() {
    uint8_t lines = 0;
    for (int8_t y = (int8_t)PLAY_H - 1; y >= 0; --y) {
      bool full = true;
      for (uint8_t x = 0; x < W; ++x) {
        if (board[y][x] == 0) { full = false; break; }
      }
      if (!full) continue;

      lines++;
      for (int8_t yy = y; yy > 0; --yy) {
        for (uint8_t x = 0; x < W; ++x) board[yy][x] = board[yy - 1][x];
      }
      for (uint8_t x = 0; x < W; ++x) board[0][x] = 0;
      y++; // re-check shifted row
    }
    return lines;
  }

  static uint32_t classicLineClearScore(uint8_t lines, uint8_t lvl) {
    uint32_t base = 0;
    switch (lines) {
      case 1: base = 40; break;
      case 2: base = 100; break;
      case 3: base = 300; break;
      case 4: base = 1200; break;
      default: base = 0; break;
    }
    return base * (uint32_t)(lvl + 1);
  }

  void updateLevelOnCleared(uint8_t cleared) {
    if (cleared == 0) return;
    totalLinesCleared += cleared;

    uint8_t newLevel = (uint8_t)(totalLinesCleared / LINES_PER_LEVEL);
    if (newLevel <= level) return;

    level = newLevel;

    uint32_t candidate = (uint32_t)BASE_FALL_MS;
    if ((uint32_t)level * FALL_DECREMENT >= candidate) {
      fallDelayMs = MIN_FALL_MS;
    } else {
      uint32_t v = candidate - (uint32_t)level * FALL_DECREMENT;
      fallDelayMs = (uint16_t)max((uint32_t)MIN_FALL_MS, v);
    }
  }

  void applyLineClearScoreAndLevel(uint8_t cleared) {
    if (cleared == 0) return;
    score += classicLineClearScore(cleared, level);
    updateLevelOnCleared(cleared);
  }

  void lockPiece() {
    uint16_t mask = shapeMask(curPiece.type, curPiece.rot);
    int8_t minPlacedRow = PLAY_H;

    for (uint8_t cy = 0; cy < 4; ++cy) for (uint8_t cx = 0; cx < 4; ++cx) {
      if (!maskCell(mask, cx, cy)) continue;

      int8_t bx = curX + (int8_t)cx;
      int8_t by = curY + (int8_t)cy;

      if (by >= 0 && by < (int8_t)PLAY_H && bx >= 0 && bx < (int8_t)W) {
        board[by][bx] = (uint8_t)(curPiece.type + 1);
        if (by < minPlacedRow) minPlacedRow = by;
      } else {
        if (by < minPlacedRow) minPlacedRow = by;
      }
    }

    // top-out
    if (minPlacedRow <= 0) gameOver = true;
  }

  void spawnNext() {
    curPiece.type = nextPiece.type;
    curPiece.rot = 0;
    curX = 3;
    curY = 0;

    nextPiece.type = (int8_t)random(0, 7);
    nextPiece.rot = 0;

    holdLocked = false;
    tFall = millis();

    if (!validAt(curX, curY, curPiece.rot)) gameOver = true;
  }

  void afterLockResolve() {
    uint8_t cleared = clearLines();
    applyLineClearScoreAndLevel(cleared);
    spawnNext();
  }

  void doHold() {
    if (holdLocked) return;

    if (holdType < 0) {
      holdType = curPiece.type;
      curPiece.type = nextPiece.type;
      curPiece.rot = 0;
      curX = 3; curY = 0;
      nextPiece.type = (int8_t)random(0,7);
      nextPiece.rot = 0;
    } else {
      int8_t tmp = holdType;
      holdType = curPiece.type;
      curPiece.type = tmp;
      curPiece.rot = 0;
      curX = 3; curY = 0;
    }

    holdLocked = true;
    if (!validAt(curX, curY, curPiece.rot)) gameOver = true;
    tFall = millis();
  }

  void tryRotateTo(uint8_t nr) {
    const int8_t kicks[] = {0, -1, 1, -2, 2};
    for (uint8_t i = 0; i < sizeof(kicks); ++i) {
      int8_t nx = curX + kicks[i];
      if (validAt(nx, curY, nr)) { curX = nx; curPiece.rot = nr; return; }
    }
  }

  void rotateRight() { tryRotateTo((curPiece.rot + 1) & 3); }
  void rotateLeft()  { tryRotateTo((curPiece.rot + 3) & 3); }

  bool tryMove(int8_t dx, int8_t dy) {
    int8_t nx = curX + dx;
    int8_t ny = curY + dy;

    if (validAt(nx, ny, curPiece.rot)) {
      curX = nx; curY = ny;
      return true;
    }

    if (dy == 1) {
      lockPiece();
      if (!gameOver) afterLockResolve();
    }
    return false;
  }

  uint16_t currentFallDelay(bool downHeld) const {
    if (!downHeld) return fallDelayMs;
    uint32_t div = (uint32_t)fallDelayMs / (uint32_t)SOFT_DROP_DIVISOR;
    uint16_t candidate = (uint16_t)max((uint32_t)SOFT_DROP_MIN_MS, div);
    return (candidate < fallDelayMs) ? candidate : fallDelayMs;
  }

  int8_t computeGhostY() const {
    int8_t gy = curY;
    while (validAt(curX, gy + 1, curPiece.rot)) gy++;
    return gy;
  }

  void reset(Renderer& r) {
    clearBoard();
    gameOver = false;
    score = 0;
    totalLinesCleared = 0;
    level = 0;
    fallDelayMs = BASE_FALL_MS;

    holdType = -1;
    holdLocked = false;

    nextPiece.type = (int8_t)random(0, 7);
    nextPiece.rot = 0;

    curPiece.type = nextPiece.type;
    curPiece.rot = 0;

    nextPiece.type = (int8_t)random(0, 7);
    nextPiece.rot = 0;

    curX = 3;
    curY = 0;

    tFall = millis();
    r.setScoreDigits(score);
  }

  void update(const InputState& in, int8_t repeatDx, uint32_t now, Renderer& r) {
    if (gameOver) return;

    // hold (edge)
    if (in.holdPressed) doHold();

    // rotate (edge)
    if (in.rotLeftPressed) rotateLeft();
    if (in.rotRightPressed) rotateRight();

    // horizontal movement via joystick repeat
    if (repeatDx != 0) tryMove(repeatDx, 0);

    // gravity / soft drop
    uint16_t fallMs = currentFallDelay(in.downHeld);
    if (now - tFall >= fallMs) {
      tFall = now;

      bool movedDown = tryMove(0, 1);

      // soft drop scoring
      if (movedDown && in.downHeld) {
        score += 1;
        r.setScoreDigits(score);
      }
    }

    r.setScoreDigits(score);
  }

  void render(Renderer& r) {
    if (gameOver) return;

    r.clearAllToBackground();

    // locked blocks
    for (uint8_t y = 0; y < PLAY_H; ++y) {
      uint16_t pr = playRowToPixelRow(y);
      for (uint8_t x = 0; x < W; ++x) {
        uint8_t v = board[y][x];
        if (v == 0) continue;
        r.pixelGrid->setGridCellColour(pr, x, PIECE_COLORS[v - 1]);
      }
    }

    // ghost outline-ish
    int8_t gy = computeGhostY();
    if (gy >= 0) {
      uint16_t mask = shapeMask((uint8_t)curPiece.type, curPiece.rot);
      uint32_t ghostColor = dimColor(*r.strip, r.GHOST_COLOR, GHOST_PERCENT);

      for (uint8_t cy = 0; cy < 4; ++cy) for (uint8_t cx = 0; cx < 4; ++cx) {
        if (!maskCell(mask, cx, cy)) continue;

        int8_t bx = curX + (int8_t)cx;
        int8_t by = gy + (int8_t)cy;
        if (bx < 0 || bx >= (int8_t)W) continue;
        if (by < 0 || by >= (int8_t)PLAY_H) continue;

        r.pixelGrid->setGridCellColour(playRowToPixelRow((uint8_t)by), (uint16_t)bx, ghostColor);
      }
    }

    // current piece
    {
      uint16_t mask = shapeMask((uint8_t)curPiece.type, curPiece.rot);
      for (uint8_t cy = 0; cy < 4; ++cy) for (uint8_t cx = 0; cx < 4; ++cx) {
        if (!maskCell(mask, cx, cy)) continue;

        int8_t bx = curX + (int8_t)cx;
        int8_t by = curY + (int8_t)cy;
        if (by < 0) continue;
        if (bx < 0 || bx >= (int8_t)W) continue;
        if (by >= (int8_t)PLAY_H) continue;

        r.pixelGrid->setGridCellColour(playRowToPixelRow((uint8_t)by), (uint16_t)bx, PIECE_COLORS[curPiece.type]);
      }
    }

    r.show();
  }

  bool isGameOver() const { return gameOver; }
};

const uint16_t TetrisGame::SHAPES[7][4] PROGMEM = {
  { 0x0F00, 0x2222, 0x00F0, 0x4444 }, // I
  { 0x6600, 0x6600, 0x6600, 0x6600 }, // O
  { 0x4E00, 0x4640, 0x0E40, 0x4C40 }, // T
  { 0x6C00, 0x4620, 0x06C0, 0x8C40 }, // S
  { 0xC600, 0x2640, 0x0C60, 0x4C80 }, // Z
  { 0x8E00, 0x6440, 0x0E20, 0x44C0 }, // J
  { 0x2E00, 0x4460, 0x0E80, 0xC440 }  // L
};
