/*
Tetris using PixelGrid classes (Adafruit_NeoPixel)
Reworked to avoid AVR relocation truncation by splitting large functions
and removing runtime debug prints that bloat the binary.

Make sure Pixel_Grid.h, LCD_Panel.h, LCD_Digit.h, Shape.h are present.
Install Adafruit_NeoPixel library before compiling.

Features retained:
- PREVIEW_ROWS at top have a distinct background color from the play area.
- Play area sits below preview rows.
- Hold triggered by long-press rotate.
- Hard-drop triggered by pressing left+right together.
- Score digits update when lines cleared.
- Ghost piece shown at landing position (outline, single shared ghost color).
- Level progression: speed increases every LINES_PER_LEVEL lines cleared.
*/

#define PIXEL_BUFFER_SIZE 300
#include <Adafruit_NeoPixel.h>
#include <avr/pgmspace.h>
#include <PixelGridCore.h>


#define PIN 6
#define PIN_LEFT  3
#define PIN_ROT   4
#define PIN_RIGHT 5

static const uint8_t PREVIEW_ROWS = 2;
static const uint8_t PLAY_H = 18;
static const uint8_t W = 10;
static const uint8_t MATRIX_ROWS = PREVIEW_ROWS + PLAY_H;

Adafruit_NeoPixel strip(PIXEL_BUFFER_SIZE, PIN, NEO_GRB + NEO_KHZ800);

Pixel_Grid* pixelGrid;
LCD_Panel* lcdPanel;

uint32_t PREVIEW_BG_COLOR_U32;
uint32_t PLAY_BG_COLOR_U32;

static uint32_t GHOST_COLOR_U32;
static const uint8_t GHOST_PERCENT = 36;

static const uint16_t HOLD_MS = 500;
static const uint16_t BASE_FALL_MS = 550;
static const uint8_t LINES_PER_LEVEL = 10;
static const uint16_t FALL_DECREMENT = 40;
static const uint16_t MIN_FALL_MS = 80;

uint16_t fallDelayMs = BASE_FALL_MS;
uint32_t totalLinesCleared = 0;
uint8_t level = 0;

static const uint16_t DEBOUNCE_MS = 18;
struct Btn {
  uint8_t pin = 0;
  bool stable = false;
  bool prevStable = false;
  bool lastRaw = false;
  uint32_t lastChange = 0;

  void begin(uint8_t p) {
    pin = p;
    pinMode(pin, INPUT_PULLUP);
    stable = false;
    prevStable = false;
    lastRaw = false;
    lastChange = millis();
  }

  void update() {
    bool raw = (digitalRead(pin) == LOW);
    uint32_t now = millis();
    if (raw != lastRaw) {
      lastRaw = raw;
      lastChange = now;
      return;
    }
    if (now - lastChange >= DEBOUNCE_MS) stable = raw;
  }

  bool pressedEdge() const { return stable && !prevStable; }
  bool releasedEdge() const { return !stable && prevStable; }
  void latch() { prevStable = stable; }
};

Btn btnL, btnR, btnRot;

uint8_t board[PLAY_H][W];

static const uint16_t SHAPES[7][4] PROGMEM = {
  { 0x0F00, 0x2222, 0x00F0, 0x4444 }, // I
  { 0x6600, 0x6600, 0x6600, 0x6600 }, // O
  { 0x4E00, 0x4640, 0x0E40, 0x4C40 }, // T
  { 0x6C00, 0x4620, 0x06C0, 0x8C40 }, // S
  { 0xC600, 0x2640, 0x0C60, 0x4C80 }, // Z
  { 0x8E00, 0x6440, 0x0E20, 0x44C0 }, // J
  { 0x2E00, 0x4460, 0x0E80, 0xC440 }  // L
};

struct Piece {
  int8_t type;
  uint8_t rot;
};

Piece curPiece;
int8_t curX = 3;
int8_t curY = 0;

bool gameOver = false;
uint32_t score = 0;
static uint32_t tFall = 0;

Piece nextPiece;
int8_t holdType = -1;
bool holdLocked = false;

uint32_t PIECE_COLORS_U32[7];

static inline uint16_t shapeMask(uint8_t type, uint8_t rot) {
  return pgm_read_word(&SHAPES[type][rot & 3]);
}
static inline bool maskCell(uint16_t mask, uint8_t cx, uint8_t cy) {
  uint8_t bitIndex = (uint8_t)(15 - (cy * 4 + cx));
  return (mask >> bitIndex) & 1;
}

static inline uint16_t playRowToPixelRow(uint8_t logicalRow) {
  return (uint16_t)(MATRIX_ROWS - 1 - (PREVIEW_ROWS + logicalRow));
}
static inline uint16_t previewRowToPixelRow(uint8_t p) { return (uint16_t)(MATRIX_ROWS - 1 - p); }

static inline uint8_t colR(uint32_t c) { return (uint8_t)((c >> 16) & 0xFF); }
static inline uint8_t colG(uint32_t c) { return (uint8_t)((c >> 8) & 0xFF); }
static inline uint8_t colB(uint32_t c) { return (uint8_t)(c & 0xFF); }
static inline uint32_t dimColor(uint32_t color, uint8_t percent) {
  uint8_t r = (uint8_t)((uint16_t)colR(color) * percent / 100);
  uint8_t g = (uint8_t)((uint16_t)colG(color) * percent / 100);
  uint8_t b = (uint8_t)((uint16_t)colB(color) * percent / 100);
  return strip.Color(r, g, b);
}

static void clearBoard() {
  for (uint8_t y = 0; y < PLAY_H; y++)
    for (uint8_t x = 0; x < W; x++)
      board[y][x] = 0;
}

static void debugPrintBoard() { /* intentionally empty to reduce binary size */ }

static uint8_t clearLines() {
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
    y++;
  }
  return lines;
}

static void updateLevelOnCleared(uint8_t cleared) {
  if (cleared == 0) return;
  totalLinesCleared += cleared;
  uint8_t newLevel = (uint8_t)(totalLinesCleared / LINES_PER_LEVEL);
  if (newLevel > level) {
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
}

static bool validAtParams(uint8_t type, uint8_t rot, int8_t nx, int8_t ny) {
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
static bool validAt(int8_t nx, int8_t ny, uint8_t nrot) { return validAtParams((uint8_t)curPiece.type, nrot, nx, ny); }

static void spawnPiece() {
  if (nextPiece.type < 0 || nextPiece.type > 6) curPiece.type = (int8_t)random(0,7);
  else curPiece.type = nextPiece.type;
  curPiece.rot = 0; curX = 3; curY = 0;
  nextPiece.type = (int8_t)random(0,7); nextPiece.rot = 0;
  holdLocked = false;
  if (!validAt(curX, curY, curPiece.rot)) gameOver = true;
}

static void lockPiece() {
  uint16_t mask = shapeMask(curPiece.type, curPiece.rot);
  int8_t minPlacedRow = PLAY_H;
  for (uint8_t cy = 0; cy < 4; cy++) for (uint8_t cx = 0; cx < 4; cx++) {
    if (!maskCell(mask, cx, cy)) continue;
    int8_t bx = curX + (int8_t)cx; int8_t by = curY + (int8_t)cy;
    if (by >= 0 && by < (int8_t)PLAY_H && bx >= 0 && bx < (int8_t)W) {
      board[by][bx] = (uint8_t)(curPiece.type + 1);
      if (by < minPlacedRow) minPlacedRow = by;
    } else {
      if (by < minPlacedRow) minPlacedRow = by;
    }
  }
  if (minPlacedRow <= 0) { gameOver = true; Serial.println(F("Game over")); }
}

static void doHold() {
  if (holdLocked) return;
  if (holdType < 0) {
    holdType = curPiece.type;
    curPiece.type = nextPiece.type; curPiece.rot = 0; curX = 3; curY = 0;
    nextPiece.type = (int8_t)random(0,7); nextPiece.rot = 0;
  } else {
    int8_t tmp = holdType; holdType = curPiece.type; curPiece.type = tmp; curPiece.rot = 0; curX = 3; curY = 0;
  }
  holdLocked = true;
  if (!validAt(curX, curY, curPiece.rot)) gameOver = true;
  tFall = millis();
}

static void hardDrop() {
  while (validAt(curX, curY + 1, curPiece.rot)) curY++;
  lockPiece();
  uint8_t cleared = clearLines();
  if (cleared > 0) { score += (uint32_t)cleared * 100UL; updateScoreDigits(score); updateLevelOnCleared(cleared); }
  curPiece.type = nextPiece.type; curPiece.rot = 0; curX = 3; curY = 0;
  nextPiece.type = (int8_t)random(0,7); nextPiece.rot = 0; holdLocked = false; tFall = millis();
}

static void tryMove(int8_t dx, int8_t dy) {
  int8_t nx = curX + dx; int8_t ny = curY + dy;
  if (validAt(nx, ny, curPiece.rot)) { curX = nx; curY = ny; return; }
  if (dy == 1) {
    lockPiece();
    uint8_t cleared = clearLines();
    if (cleared > 0) { score += (uint32_t)cleared * 100UL; updateScoreDigits(score); updateLevelOnCleared(cleared); }
    curPiece.type = nextPiece.type; curPiece.rot = 0; curX = 3; curY = 0; nextPiece.type = (int8_t)random(0,7); nextPiece.rot = 0; holdLocked = false;
  }
}

static void tryRotate() {
  uint8_t nr = (curPiece.rot + 1) & 3; const int8_t kicks[] = {0,-1,1,-2,2};
  for (uint8_t i=0;i<sizeof(kicks);i++) { int8_t nx = curX + kicks[i]; if (validAt(nx, curY, nr)) { curX = nx; curPiece.rot = nr; return; } }
}

// --- Draw helpers split to smaller chunks ---
// Draw previews and background
static void drawBackgroundRows() {
  for (uint8_t p = 0; p < PREVIEW_ROWS; ++p) { uint16_t pixelRow = previewRowToPixelRow(p); for (uint8_t col = 0; col < W; ++col) pixelGrid->setGridCellColour(pixelRow, col, PREVIEW_BG_COLOR_U32); }
  for (uint8_t pr = 0; pr < PLAY_H; ++pr) { uint16_t pixelRow = playRowToPixelRow(pr); for (uint8_t col = 0; col < W; ++col) pixelGrid->setGridCellColour(pixelRow, col, PLAY_BG_COLOR_U32); }
}
static void drawPreviews() {
  if (holdType >= 0) {
    uint16_t holdMask = shapeMask((uint8_t)holdType, 0);
    for (uint8_t py=0; py<4; ++py) for (uint8_t px=0; px<4; ++px) if (maskCell(holdMask, px, py)) {
      int pr = (int)py, pc = (int)px;
      if (pr>=0 && pr<PREVIEW_ROWS && pc>=0 && pc<W) pixelGrid->setGridCellColour(previewRowToPixelRow((uint8_t)pr),(uint16_t)pc,PIECE_COLORS_U32[holdType]);
    }
  }
  uint16_t nextMask = shapeMask((uint8_t)nextPiece.type,0);
  for (uint8_t py=0; py<4; ++py) for (uint8_t px=0; px<4; ++px) if (maskCell(nextMask, px, py)) {
    int pr = (int)py, pc = 6 + (int)px;
    if (pr>=0 && pr<PREVIEW_ROWS && pc>=0 && pc<W) pixelGrid->setGridCellColour(previewRowToPixelRow((uint8_t)pr),(uint16_t)pc,PIECE_COLORS_U32[nextPiece.type]);
  }
}

// Split locked-blocks into two smaller functions to avoid relocation issues
static void drawLockedBlocks_part1() {
  // rows 0 .. 8 (first half)
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
static void drawLockedBlocks_part2() {
  // rows mid .. PLAY_H-1 (second half)
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
static void drawLockedBlocks() { drawLockedBlocks_part1(); drawLockedBlocks_part2(); }

static int8_t computeGhostY() { int8_t gy = curY; while (validAt(curX, gy + 1, curPiece.rot)) gy++; return gy; }

static void drawGhostPiece() {
  int8_t gy = computeGhostY();
  if (gy < 0) return;
  uint16_t mask = shapeMask((uint8_t)curPiece.type, curPiece.rot);
  uint32_t ghostColor = dimColor(GHOST_COLOR_U32, GHOST_PERCENT);
  for (uint8_t cy=0; cy<4; ++cy) for (uint8_t cx=0; cx<4; ++cx) {
    if (!maskCell(mask, cx, cy)) continue;
    int8_t bx = curX + (int8_t)cx, by = gy + (int8_t)cy;
    if (by < 0 || bx < 0 || bx >= W || by >= PLAY_H) continue;
    bool draw = false; const int8_t nx[4]={-1,1,0,0}; const int8_t ny[4]={0,0,-1,1};
    for (uint8_t n=0;n<4;++n) { int8_t mx=(int8_t)cx+nx[n], my=(int8_t)cy+ny[n]; if (mx<0||mx>=4||my<0||my>=4) { draw=true; break; } if (!maskCell(mask,(uint8_t)mx,(uint8_t)my)) { draw=true; break; } }
    if (!draw) continue;
    uint16_t pixelRow = playRowToPixelRow((uint8_t)by);
    pixelGrid->setGridCellColour(pixelRow, (uint16_t)bx, ghostColor);
  }
}

static void drawCurrentPiece() {
  uint16_t mask = shapeMask((uint8_t)curPiece.type, curPiece.rot);
  for (uint8_t cy=0; cy<4; ++cy) for (uint8_t cx=0; cx<4; ++cx) {
    if (!maskCell(mask, cx, cy)) continue;
    int8_t bx = curX + (int8_t)cx, by = curY + (int8_t)cy;
    if (by >= 0 && bx >= 0 && bx < W && by < PLAY_H) {
      uint16_t pixelRow = playRowToPixelRow((uint8_t)by);
      pixelGrid->setGridCellColour(pixelRow, (uint16_t)bx, PIECE_COLORS_U32[curPiece.type]);
    }
  }
}

static void finalizeDigitsAndShow() {
  lcdPanel->render();
  pixelGrid->render();
  strip.show();
}

static void renderFrame() {
  if (gameOver) {
    for (uint8_t p=0;p<PREVIEW_ROWS;++p) for (uint8_t col=0;col<W;++col) pixelGrid->setGridCellColour(previewRowToPixelRow(p), col, strip.Color(30,0,0));
    for (uint8_t pr=0;pr<PLAY_H;++pr) for (uint8_t col=0;col<W;++col) pixelGrid->setGridCellColour(playRowToPixelRow(pr), col, strip.Color(30,0,0));
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

static void updateScoreDigits(uint32_t s) {
  char tmp[7]; char out[6];
  snprintf(tmp, sizeof(tmp), "%6lu", (unsigned long)s);
  for (uint8_t i=0;i<6;++i) out[i]=tmp[i];
  lcdPanel->changeCharArray(out);
}

static void resetGame() {
  clearBoard();
  gameOver=false; score=0; totalLinesCleared=0; level=0; fallDelayMs=BASE_FALL_MS;
  nextPiece.type = (int8_t)random(0,7); nextPiece.rot=0;
  curPiece.type = nextPiece.type; curPiece.rot=0;
  nextPiece.type = (int8_t)random(0,7);
  curX=3; curY=0; holdType=-1; holdLocked=false;
  tFall = millis();
  updateScoreDigits(score);
}

void setup() {
  randomSeed(analogRead(A0));
  Serial.begin(115200);
  strip.begin(); strip.show();
  PREVIEW_BG_COLOR_U32 = strip.Color(80,80,120);
  PLAY_BG_COLOR_U32 = strip.Color(6,6,12);
  GHOST_COLOR_U32 = strip.Color(120,120,120);
  pixelGrid = new Pixel_Grid(&strip, 0, MATRIX_ROWS, W);
  lcdPanel = new LCD_Panel(&strip, 214, 6, strip.Color(255,255,255));
  PIECE_COLORS_U32[0]=strip.Color(0,220,220);
  PIECE_COLORS_U32[1]=strip.Color(230,230,0);
  PIECE_COLORS_U32[2]=strip.Color(180,0,220);
  PIECE_COLORS_U32[3]=strip.Color(0,220,0);
  PIECE_COLORS_U32[4]=strip.Color(220,0,0);
  PIECE_COLORS_U32[5]=strip.Color(0,0,220);
  PIECE_COLORS_U32[6]=strip.Color(255,120,0);
  btnL.begin(PIN_LEFT); btnR.begin(PIN_RIGHT); btnRot.begin(PIN_ROT);
  updateScoreDigits(0); lcdPanel->render();
  drawBackgroundRows(); pixelGrid->render(); strip.show();
  resetGame();
}

void loop() {
  btnL.update(); btnR.update(); btnRot.update();
  static uint32_t rotPressTime = 0; static bool rotLongTriggered = false;
  if (gameOver) {
    if (btnRot.pressedEdge()) resetGame();
    btnL.latch(); btnR.latch(); btnRot.latch();
    renderFrame(); delay(10); return;
  }
  bool L_edge = btnL.pressedEdge(); bool R_edge = btnR.pressedEdge();
  bool L_down = btnL.stable; bool R_down = btnR.stable;
  if (btnRot.pressedEdge()) { rotPressTime = millis(); rotLongTriggered = false; }
  if (btnRot.stable && rotPressTime != 0 && !rotLongTriggered) {
    if (millis() - rotPressTime >= HOLD_MS) { doHold(); rotLongTriggered = true; btnRot.latch(); }
  }
  if (btnRot.releasedEdge()) { if (!rotLongTriggered) tryRotate(); rotPressTime = 0; rotLongTriggered = false; }
  if ((L_edge && R_down) || (R_edge && L_down) || (L_edge && R_edge)) { hardDrop(); btnL.latch(); btnR.latch(); }
  else { if (L_edge) tryMove(-1,0); if (R_edge) tryMove(1,0); }
  uint32_t now = millis(); if (now - tFall >= fallDelayMs) { tFall = now; tryMove(0,1); }
  renderFrame();
  btnL.latch(); btnR.latch(); btnRot.latch();
  delay(10);
}