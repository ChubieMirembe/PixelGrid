/*
Tetris using PixelGrid classes (Adafruit_NeoPixel)
Replace Tetris.ino with this file in the pixeltest/ folder.
Make sure Pixel_Grid.h, LCD_Panel.h, LCD_Digit.h, Shape.h are present.
Install Adafruit_NeoPixel library before compiling.

This version:
- PREVIEW_ROWS at top have a distinct background color from the play area.
- Play area sits below preview rows.
- Hold is triggered by pressing and holding the rotate button (500 ms).
- Hard-drop triggered by pressing left+right together.
- Score digits updated immediately when lines are cleared.
*/

#define PIXEL_BUFFER_SIZE 300   // set to total LEDs in your chain (matrix + gap + digits)
#include <Adafruit_NeoPixel.h>
#include "Pixel_Grid.h"
#include "LCD_Panel.h"
#include "Shape.h"

#include <avr/pgmspace.h> // for PROGMEM access on AVR

// Hardware pins
#define PIN 6               // data pin for NeoPixel chain (change if needed)
#define PIN_LEFT  3
#define PIN_ROT   4
#define PIN_RIGHT 5

// Layout
static const uint8_t PREVIEW_ROWS = 2;   // number of rows reserved at top for previews
static const uint8_t PLAY_H = 18;        // playable rows
static const uint8_t W = 10;             // columns
static const uint8_t MATRIX_ROWS = PREVIEW_ROWS + PLAY_H; // total physical rows (should be 20)

// Adafruit strip
Adafruit_NeoPixel strip(PIXEL_BUFFER_SIZE, PIN, NEO_GRB + NEO_KHZ800);

// PixelGrid + LCD panel
Pixel_Grid* pixelGrid;
LCD_Panel* lcdPanel;

// background colors (set in setup after strip.begin())
uint32_t PREVIEW_BG_COLOR_U32;
uint32_t PLAY_BG_COLOR_U32;

// hold threshold (ms)
static const uint16_t HOLD_MS = 500;

// simple edge-detect + debounce button
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
    bool raw = (digitalRead(pin) == LOW); // active LOW because of INPUT_PULLUP
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

// Tetris state (play area only)
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

Piece curPiece;        // current falling piece type/rotation (position stored separately)
int8_t curX = 3;       // column
int8_t curY = 0;       // row in play-area coordinates (0..PLAY_H-1)

bool gameOver = false;
uint32_t score = 0;
static uint32_t tFall = 0;
static const uint16_t FALL_MS = 550;

// previews / hold
Piece nextPiece;
int8_t holdType = -1;      // -1 none, otherwise 0..6
bool holdLocked = false;   // locked until next spawn

// piece colors (Adafruit strip.Color values)
uint32_t PIECE_COLORS_U32[7];

static inline uint16_t shapeMask(uint8_t type, uint8_t rot) {
  return pgm_read_word(&SHAPES[type][rot & 3]);
}

static inline bool maskCell(uint16_t mask, uint8_t cx, uint8_t cy) {
  uint8_t bitIndex = (uint8_t)(15 - (cy * 4 + cx));
  return (mask >> bitIndex) & 1;
}

// Map a play-area logical row (0 = top of play area) to Pixel_Grid row index
// Pixel_Grid row index = MATRIX_ROWS - 1 - (PREVIEW_ROWS + logicalRow)
static inline uint16_t playRowToPixelRow(uint8_t logicalRow) {
  return (uint16_t)(MATRIX_ROWS - 1 - (PREVIEW_ROWS + logicalRow));
}

// Map a preview logical row p (0..PREVIEW_ROWS-1) to Pixel_Grid row index
// Pixel_Grid row index = MATRIX_ROWS - 1 - p
static inline uint16_t previewRowToPixelRow(uint8_t p) {
  return (uint16_t)(MATRIX_ROWS - 1 - p);
}

// Clear board (play area)
static void clearBoard() {
  for (uint8_t y = 0; y < PLAY_H; y++)
    for (uint8_t x = 0; x < W; x++)
      board[y][x] = 0;
}

// debug print board (logical top->bottom)
static void debugPrintBoard() {
  Serial.println(F("board (logical play rows top->bottom):"));
  for (uint8_t y = 0; y < PLAY_H; ++y) {
    for (uint8_t x = 0; x < W; ++x) {
      Serial.print(board[y][x]);
      Serial.print(' ');
    }
    Serial.println();
  }
  Serial.println();
}

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

    y++; // re-check this row
  }

  return lines;
}

// Spawn uses nextPiece if pre-chosen; sets nextPiece afterwards
static void spawnPiece() {
  // current <- next (if valid) otherwise random
  if (nextPiece.type < 0 || nextPiece.type > 6) {
    curPiece.type = (int8_t)random(0, 7);
  } else {
    curPiece.type = nextPiece.type;
  }
  curPiece.rot = 0;
  curX = 3;
  curY = 0;

  // choose new next
  nextPiece.type = (int8_t)random(0, 7);
  nextPiece.rot = 0;

  holdLocked = false;

  // if spawn invalid, mark game over
  if (!validAt(curX, curY, curPiece.rot)) gameOver = true;
}

// Validate whether piece of given parameters fits
static bool validAtParams(uint8_t type, uint8_t rot, int8_t nx, int8_t ny) {
  uint16_t mask = shapeMask(type, rot);
  for (uint8_t cy = 0; cy < 4; cy++) {
    for (uint8_t cx = 0; cx < 4; cx++) {
      if (!maskCell(mask, cx, cy)) continue;
      int8_t bx = nx + (int8_t)cx;
      int8_t by = ny + (int8_t)cy;
      if (bx < 0 || bx >= (int8_t)W) return false;
      if (by >= (int8_t)PLAY_H) return false; // below play area
      if (by >= 0 && board[by][bx] != 0) return false;
    }
  }
  return true;
}

static bool validAt(int8_t nx, int8_t ny, uint8_t nrot) {
  return validAtParams((uint8_t)curPiece.type, nrot, nx, ny);
}

static void lockPiece() {
  uint16_t mask = shapeMask(curPiece.type, curPiece.rot);

  // Track the smallest (highest) row we wrote to the board
  int8_t minPlacedRow = PLAY_H; // large sentinel

  for (uint8_t cy = 0; cy < 4; cy++) {
    for (uint8_t cx = 0; cx < 4; cx++) {
      if (!maskCell(mask, cx, cy)) continue;
      int8_t bx = curX + (int8_t)cx;
      int8_t by = curY + (int8_t)cy;
      // If cell is within play area, write it
      if (by >= 0 && by < (int8_t)PLAY_H && bx >= 0 && bx < (int8_t)W) {
        board[by][bx] = (uint8_t)(curPiece.type + 1);
        if (by < minPlacedRow) minPlacedRow = by;
      } else {
        // If any part of the piece is above the visible play area (< 0),
        // treat that as having reached the top — mark minPlacedRow accordingly.
        if (by < minPlacedRow) minPlacedRow = by;
      }
    }
  }

  // If any placed cell is at or above the topmost row (by <= 0), end the game.
  // Using <=0 treats a piece that partly sits above the playfield as game over.
  if (minPlacedRow <= 0) {
    gameOver = true;
    Serial.println(F("Game over: block locked at top"));
  }
}

static void doHold() {
  if (holdLocked) return;

  if (holdType < 0) {
    holdType = curPiece.type;
    // spawn next
    curPiece.type = nextPiece.type;
    curPiece.rot = 0;
    curX = 3;
    curY = 0;
    nextPiece.type = (int8_t)random(0, 7);
    nextPiece.rot = 0;
  } else {
    int8_t tmp = holdType;
    holdType = curPiece.type;
    curPiece.type = tmp;
    curPiece.rot = 0;
    curX = 3;
    curY = 0;
    // nextPiece unaffected
  }

  holdLocked = true;
  // If new current piece invalid -> game over
  if (!validAt(curX, curY, curPiece.rot)) gameOver = true;
  // Reset fall timer
  tFall = millis();
}

static void hardDrop() {
  while (validAt(curX, curY + 1, curPiece.rot)) {
    curY++;
  }
  lockPiece();
  uint8_t cleared = clearLines();
  if (cleared > 0) {
    score += (uint32_t)cleared * 100UL;
    Serial.print(F("hardDrop cleared: "));
    Serial.print(cleared);
    Serial.print(F("  score: "));
    Serial.println(score);
    debugPrintBoard();
    updateScoreDigits(score); // update digits
    // optionally push update immediately:
    // lcdPanel->render(); pixelGrid->render(); strip.show();
  }
  // spawn next
  curPiece.type = nextPiece.type;
  curPiece.rot = 0;
  curX = 3; curY = 0;
  nextPiece.type = (int8_t)random(0, 7);
  nextPiece.rot = 0;
  holdLocked = false;
  tFall = millis();
}

// Movement / rotation logic
static void tryMove(int8_t dx, int8_t dy) {
  int8_t nx = curX + dx;
  int8_t ny = curY + dy;

  if (validAt(nx, ny, curPiece.rot)) {
    curX = nx;
    curY = ny;
    return;
  }

  // If down failed: lock -> clear -> score -> spawn next
  if (dy == 1) {
    lockPiece();
    Serial.println(F("locked piece"));
    uint8_t cleared = clearLines();
    if (cleared > 0) {
      score += (uint32_t)cleared * 100UL;
      Serial.print(F("cleared: "));
      Serial.print(cleared);
      Serial.print(F("  score: "));
      Serial.println(score);
      debugPrintBoard();
      updateScoreDigits(score); // update digits after clearing
      // optionally immediate render: lcdPanel->render(); pixelGrid->render(); strip.show();
    }
    // spawn next
    curPiece.type = nextPiece.type;
    curPiece.rot = 0;
    curX = 3; curY = 0;
    nextPiece.type = (int8_t)random(0, 7);
    nextPiece.rot = 0;
    holdLocked = false;
  }
}

static void tryRotate() {
  uint8_t nr = (curPiece.rot + 1) & 3;
  const int8_t kicks[] = { 0, -1, 1, -2, 2 };
  for (uint8_t i = 0; i < sizeof(kicks); i++) {
    int8_t nx = curX + kicks[i];
    if (validAt(nx, curY, nr)) {
      curX = nx;
      curPiece.rot = nr;
      return;
    }
  }
}

// Fill background rows with distinct preview/play colors
static void fillBackgroundRows() {
  // preview rows
  for (uint8_t p = 0; p < PREVIEW_ROWS; ++p) {
    uint16_t pixelRow = previewRowToPixelRow(p);
    for (uint8_t col = 0; col < W; ++col) {
      pixelGrid->setGridCellColour(pixelRow, col, PREVIEW_BG_COLOR_U32);
    }
  }
  // play rows
  for (uint8_t pr = 0; pr < PLAY_H; ++pr) {
    uint16_t pixelRow = playRowToPixelRow(pr);
    for (uint8_t col = 0; col < W; ++col) {
      pixelGrid->setGridCellColour(pixelRow, col, PLAY_BG_COLOR_U32);
    }
  }
}

// Draw a 4x4 mask into the preview area (top PREVIEW_ROWS) at preview column offset (0.. W-4)
static void drawMaskToPreview(uint16_t mask, uint8_t colorIndex, uint8_t previewRow, uint8_t previewCol) {
  for (uint8_t py = 0; py < 4; ++py) {
    for (uint8_t px = 0; px < 4; ++px) {
      if (!maskCell(mask, px, py)) continue;
      int pr = (int)previewRow + (int)py; // 0..PREVIEW_ROWS-1 (should be within preview area)
      int pc = (int)previewCol + (int)px;
      if (pr < 0 || pr >= (int)PREVIEW_ROWS || pc < 0 || pc >= (int)W) continue;
      uint16_t pixelRow = previewRowToPixelRow((uint8_t)pr);
      pixelGrid->setGridCellColour(pixelRow, (uint16_t)pc, PIECE_COLORS_U32[colorIndex]);
    }
  }
}

// Update the LCD digits with score (right-aligned)
static void updateScoreDigits(uint32_t s) {
  char tmp[7];
  char out[6];
  snprintf(tmp, sizeof(tmp), "%6lu", (unsigned long)s);
  for (uint8_t i = 0; i < 6; ++i) out[i] = tmp[i];
  lcdPanel->changeCharArray(out);
}

static void renderFrame() {
  if (gameOver) {
    // set dark red background for gameover
    for (uint8_t p = 0; p < PREVIEW_ROWS; ++p) {
      uint16_t pixelRow = previewRowToPixelRow(p);
      for (uint8_t col = 0; col < W; ++col) pixelGrid->setGridCellColour(pixelRow, col, strip.Color(30, 0, 0));
    }
    for (uint8_t pr = 0; pr < PLAY_H; ++pr) {
      uint16_t pixelRow = playRowToPixelRow(pr);
      for (uint8_t col = 0; col < W; ++col) pixelGrid->setGridCellColour(pixelRow, col, strip.Color(30, 0, 0));
    }
    lcdPanel->render();
    pixelGrid->render();
    strip.show();
    return;
  }

  // fill backgrounds: preview rows distinct from play rows
  fillBackgroundRows();

  // draw previews: hold at top-left (preview cols 0..3), next at top-right (cols 6..9)
  if (holdType >= 0) {
    uint16_t holdMask = shapeMask((uint8_t)holdType, 0);
    drawMaskToPreview(holdMask, (uint8_t)holdType, 0, 0); // top-left preview
  }
  // draw next (use rotation 0)
  uint16_t nextMask = shapeMask((uint8_t)nextPiece.type, 0);
  drawMaskToPreview(nextMask, (uint8_t)nextPiece.type, 0, 6); // top-right preview

  // locked blocks (play-area)
  for (uint8_t y = 0; y < PLAY_H; y++) {
    for (uint8_t x = 0; x < W; x++) {
      uint8_t v = board[y][x];
      if (v == 0) continue;
      // map play-row y -> pixel row
      uint16_t pixelRow = playRowToPixelRow(y);
      pixelGrid->setGridCellColour(pixelRow, x, PIECE_COLORS_U32[v - 1]);
    }
  }

  // current piece (play-area)
  uint16_t mask = shapeMask((uint8_t)curPiece.type, curPiece.rot);
  for (uint8_t cy = 0; cy < 4; cy++) {
    for (uint8_t cx = 0; cx < 4; cx++) {
      if (!maskCell(mask, cx, cy)) continue;
      int8_t bx = curX + (int8_t)cx;
      int8_t by = curY + (int8_t)cy;
      if (by >= 0 && bx >= 0 && bx < W && by < PLAY_H) {
        uint16_t pixelRow = playRowToPixelRow((uint8_t)by);
        pixelGrid->setGridCellColour(pixelRow, (uint16_t)bx, PIECE_COLORS_U32[curPiece.type]);
      }
    }
  }

  // digits (score)
  lcdPanel->render();

  // render buffers into strip and show once
  pixelGrid->render();
  strip.show();
}

static void resetGame() {
  clearBoard();
  gameOver = false;
  score = 0;
  // pick initial next/current
  nextPiece.type = (int8_t)random(0,7);
  nextPiece.rot = 0;
  curPiece.type = nextPiece.type;
  curPiece.rot = 0;
  nextPiece.type = (int8_t)random(0,7);
  curX = 3;
  curY = 0;
  holdType = -1;
  holdLocked = false;
  tFall = millis();
  updateScoreDigits(score);
}

void setup() {
  randomSeed(analogRead(A0));
  Serial.begin(115200);   // enable debug output

  // strip & classes
  strip.begin();
  strip.show(); // clear

  // choose background colors (preview vs play). Adjust RGB values as desired:
  PREVIEW_BG_COLOR_U32 = strip.Color(80, 80, 120); // preview rows background (brighter)
  PLAY_BG_COLOR_U32 = strip.Color(6, 6, 12);      // play area background (original)

  // Pixel_Grid expects number of rows = MATRIX_ROWS
  pixelGrid = new Pixel_Grid(&strip, 0, MATRIX_ROWS, W);      // start index 0 for matrix
  lcdPanel  = new LCD_Panel(&strip, 214, 6, strip.Color(255,255,255)); // digits start at 214 (adjust if needed)

  // initialize piece colors
  PIECE_COLORS_U32[0] = strip.Color(0, 220, 220);   // I
  PIECE_COLORS_U32[1] = strip.Color(230, 230, 0);   // O
  PIECE_COLORS_U32[2] = strip.Color(180, 0, 220);   // T
  PIECE_COLORS_U32[3] = strip.Color(0, 220, 0);     // S
  PIECE_COLORS_U32[4] = strip.Color(220, 0, 0);     // Z
  PIECE_COLORS_U32[5] = strip.Color(0, 0, 220);     // J
  PIECE_COLORS_U32[6] = strip.Color(255, 120, 0);   // L

  // buttons
  btnL.begin(PIN_LEFT);
  btnR.begin(PIN_RIGHT);
  btnRot.begin(PIN_ROT);

  // startup visuals
  updateScoreDigits(0);
  lcdPanel->render();
  // fill the matrix with our two-tone background and show once
  fillBackgroundRows();
  pixelGrid->render();
  strip.show();

  // initialize game
  resetGame();
}

void loop() {
  // update buttons
  btnL.update();
  btnR.update();
  btnRot.update();

  // rotate long-press handling variables (static to persist between frames)
  static uint32_t rotPressTime = 0;
  static bool rotLongTriggered = false;

  if (gameOver) {
    // hold-to-restart or rotate to restart
    if (btnRot.pressedEdge()) {
      // short press for restart
      resetGame();
    }
    // ensure latches and rendering
    btnL.latch(); btnR.latch(); btnRot.latch();
    renderFrame();
    delay(10);
    return;
  }

  // Hard-drop when left+right pressed together (trigger once when second press occurs)
  bool L_edge = btnL.pressedEdge();
  bool R_edge = btnR.pressedEdge();
  bool L_down = btnL.stable;
  bool R_down = btnR.stable;

  // Long-press detection for rotate -> hold
  if (btnRot.pressedEdge()) {
    rotPressTime = millis();
    rotLongTriggered = false;
  }
  if (btnRot.stable && rotPressTime != 0 && !rotLongTriggered) {
    if (millis() - rotPressTime >= HOLD_MS) {
      // long hold reached -> do hold
      doHold();
      rotLongTriggered = true;
      // consume latch for rotate so short-press rotate doesn't occur
      btnRot.latch();
    }
  }
  if (btnRot.releasedEdge()) {
    // released: if we didn't trigger long press, treat as regular rotate
    if (!rotLongTriggered) {
      tryRotate();
    }
    rotPressTime = 0;
    rotLongTriggered = false;
  }

  // If left+right combo triggers hard drop
  if ((L_edge && R_down) || (R_edge && L_down) || (L_edge && R_edge)) {
    hardDrop();
    // consume the edges so we don't re-trigger while holding
    btnL.latch();
    btnR.latch();
  } else {
    // Only apply left/right movement if not used for hard drop
    if (L_edge) tryMove(-1, 0);
    if (R_edge) tryMove( 1, 0);
  }

  // Automatic fall
  uint32_t now = millis();
  if (now - tFall >= FALL_MS) {
    tFall = now;
    tryMove(0, 1);
  }

  renderFrame();

  // latch button states for next frame
  btnL.latch(); btnR.latch(); btnRot.latch();

  delay(10);
}