// Game.h
#pragma once
#include <Arduino.h>
#include <avr/pgmspace.h>
#include "Pins.h"
#include "Input.h"

// ---------- Shared state (read by Render) ----------
static uint8_t board[PLAY_H][W];

struct Piece {
  int8_t type;
  uint8_t rot;
};

static Piece curPiece;
static int8_t curX = 3;
static int8_t curY = 0;

static bool gameOver = false;
static uint32_t score = 0;

static uint16_t fallDelayMs = BASE_FALL_MS;
static uint32_t totalLinesCleared = 0;
static uint8_t level = 0;

static Piece nextPiece;
static int8_t holdType = -1;
static bool holdLocked = false;

// Repeat state
static uint32_t tFall = 0;
static uint32_t tMoveL = 0;
static uint32_t tMoveR = 0;
static bool moveLRepeating = false;
static bool moveRRepeating = false;

// ---------- Shapes ----------
static const uint16_t SHAPES[7][4] PROGMEM = {
  { 0x0F00, 0x2222, 0x00F0, 0x4444 }, // I
  { 0x6600, 0x6600, 0x6600, 0x6600 }, // O
  { 0x4E00, 0x4640, 0x0E40, 0x4C40 }, // T
  { 0x6C00, 0x4620, 0x06C0, 0x8C40 }, // S
  { 0xC600, 0x2640, 0x0C60, 0x4C80 }, // Z
  { 0x8E00, 0x6440, 0x0E20, 0x44C0 }, // J
  { 0x2E00, 0x4460, 0x0E80, 0xC440 }  // L
};

static inline uint16_t shapeMask(uint8_t type, uint8_t rot) {
  return pgm_read_word(&SHAPES[type][rot & 3]);
}
static inline bool maskCell(uint16_t mask, uint8_t cx, uint8_t cy) {
  uint8_t bitIndex = (uint8_t)(15 - (cy * 4 + cx));
  return (mask >> bitIndex) & 1;
}

// ---------- Score digits hookup (implemented in .ino) ----------
extern void updateScoreDigits(uint32_t s);

// ---------- Core helpers ----------
static inline void clearBoard() {
  for (uint8_t y = 0; y < PLAY_H; y++)
    for (uint8_t x = 0; x < W; x++)
      board[y][x] = 0;
}

static inline uint8_t clearLines() {
  uint8_t lines = 0;
  for (int8_t y = (int8_t)PLAY_H - 1; y >= 0; y--) {
    bool full = true;
    for (uint8_t x = 0; x < W; x++) {
      if (board[y][x] == 0) { full = false; break; }
    }
    if (!full) continue;

    lines++;
    for (int8_t yy = y; yy > 0; yy--) {
      for (uint8_t x = 0; x < W; x++) board[yy][x] = board[yy - 1][x];
    }
    for (uint8_t x = 0; x < W; x++) board[0][x] = 0;
    y++; // re-check same row index after shifting
  }
  return lines;
}

static inline void updateLevelOnCleared(uint8_t cleared) {
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

  Serial.print(F("Level "));
  Serial.println(level);
}

static inline uint32_t classicLineClearScore(uint8_t lines, uint8_t lvl) {
  // NES-style: base * (level+1)
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

static inline void applyLineClearScoreAndLevel(uint8_t cleared) {
  if (cleared == 0) return;

  // Score uses the level at the time of clear
  score += classicLineClearScore(cleared, level);
  updateScoreDigits(score);

  updateLevelOnCleared(cleared);
}

static inline bool validAtParams(uint8_t type, uint8_t rot, int8_t nx, int8_t ny) {
  uint16_t mask = shapeMask(type, rot);

  for (uint8_t cy = 0; cy < 4; cy++) {
    for (uint8_t cx = 0; cx < 4; cx++) {
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

static inline bool validAt(int8_t nx, int8_t ny, uint8_t nrot) {
  return validAtParams((uint8_t)curPiece.type, nrot, nx, ny);
}

static inline void lockPiece() {
  uint16_t mask = shapeMask((uint8_t)curPiece.type, curPiece.rot);
  int8_t minPlacedRow = PLAY_H;

  for (uint8_t cy = 0; cy < 4; cy++) {
    for (uint8_t cx = 0; cx < 4; cx++) {
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
  }

  if (minPlacedRow <= 0) {
    gameOver = true;
    Serial.println(F("Game over"));
  }
}

static inline void afterLockResolve() {
  uint8_t cleared = clearLines();
  applyLineClearScoreAndLevel(cleared);

  curPiece.type = nextPiece.type;
  curPiece.rot = 0;
  curX = 3; curY = 0;

  nextPiece.type = (int8_t)random(0, 7);
  nextPiece.rot = 0;

  holdLocked = false;
  tFall = millis();

  if (!validAt(curX, curY, curPiece.rot)) gameOver = true;
}

static inline void doHold() {
  if (holdLocked) return;

  if (holdType < 0) {
    holdType = curPiece.type;

    curPiece.type = nextPiece.type;
    curPiece.rot = 0;
    curX = 3; curY = 0;

    nextPiece.type = (int8_t)random(0, 7);
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

static inline void tryRotateTo(uint8_t nr) {
  const int8_t kicks[] = { 0, -1, 1, -2, 2 };
  for (uint8_t i = 0; i < (uint8_t)sizeof(kicks); i++) {
    int8_t nx = curX + kicks[i];
    if (validAt(nx, curY, nr)) { curX = nx; curPiece.rot = nr; return; }
  }
}
static inline void rotateRight() { tryRotateTo((curPiece.rot + 1) & 3); }
static inline void rotateLeft()  { tryRotateTo((curPiece.rot + 3) & 3); }

// Returns true if a movement actually happened (useful for soft-drop scoring)
static inline bool tryMove(int8_t dx, int8_t dy) {
  int8_t nx = curX + dx;
  int8_t ny = curY + dy;

  if (validAt(nx, ny, curPiece.rot)) {
    curX = nx; curY = ny;
    return true;
  }

  if (dy == 1) {
    lockPiece();
    afterLockResolve();
  }
  return false;
}

// ---------- Input handling (repeat) ----------
static inline void handleJoystickMoveRepeat() {
  uint32_t now = millis();

  // LEFT
  if (leftHeld()) {
    if (leftPressed()) {
      tryMove(-1, 0);
      tMoveL = now;
      moveLRepeating = false;
    } else {
      uint16_t waitMs = moveLRepeating ? MOVE_REPEAT_MS : MOVE_REPEAT_START_MS;
      if (now - tMoveL >= waitMs) {
        tryMove(-1, 0);
        tMoveL = now;
        moveLRepeating = true;
      }
    }
  } else if (joyL.releasedEdge()) {
    moveLRepeating = false;
  }

  // RIGHT
  if (rightHeld()) {
    if (rightPressed()) {
      tryMove(1, 0);
      tMoveR = now;
      moveRRepeating = false;
    } else {
      uint16_t waitMs = moveRRepeating ? MOVE_REPEAT_MS : MOVE_REPEAT_START_MS;
      if (now - tMoveR >= waitMs) {
        tryMove(1, 0);
        tMoveR = now;
        moveRRepeating = true;
      }
    }
  } else if (joyR.releasedEdge()) {
    moveRRepeating = false;
  }
}

static inline uint16_t currentFallDelay() {
  if (!downHeld()) return fallDelayMs;
  uint32_t div = (uint32_t)fallDelayMs / (uint32_t)SOFT_DROP_DIVISOR;
  uint16_t candidate = (uint16_t)max((uint32_t)SOFT_DROP_MIN_MS, div);
  return (candidate < fallDelayMs) ? candidate : fallDelayMs;
}

// ---------- Public “Game API” ----------
static inline void gameReset() {
  clearBoard();
  gameOver = false;

  score = 0;
  totalLinesCleared = 0;
  level = 0;
  fallDelayMs = BASE_FALL_MS;

  nextPiece.type = (int8_t)random(0, 7);
  nextPiece.rot  = 0;

  curPiece.type = nextPiece.type;
  curPiece.rot  = 0;

  nextPiece.type = (int8_t)random(0, 7);
  nextPiece.rot  = 0;

  curX = 3; curY = 0;

  holdType = -1;
  holdLocked = false;

  tFall = millis();

  tMoveL = tMoveR = millis();
  moveLRepeating = moveRRepeating = false;

  updateScoreDigits(score);
}

static inline void gameInit() {
  gameReset();
}

static inline void gameUpdate() {
  // If game over, Game does nothing (main loop decides reset)
  if (gameOver) return;

  // Hold on joystick UP (edge)
  if (holdPressed()) doHold();

  // Rotate (edge)
  if (rotateLeftPressed()) rotateLeft();
  if (rotateRightPressed()) rotateRight();

  // Joystick move with repeat
  handleJoystickMoveRepeat();

  // Gravity / soft drop
  uint32_t now = millis();
  uint16_t fallMs = currentFallDelay();
  if (now - tFall >= fallMs) {
    tFall = now;

    // Soft drop scoring: +1 per successful downward step while DOWN held
    bool moved = tryMove(0, 1);
    if (moved && downHeld()) {
      score += 1;
      updateScoreDigits(score);
    }
  }
}
