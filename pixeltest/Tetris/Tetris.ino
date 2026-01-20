/*
Tetris using PixelGrid classes (Adafruit_NeoPixel)
Replace Tetris.ino with this file in the pixeltest/ folder.
Make sure Pixel_Grid.h, LCD_Panel.h, LCD_Digit.h, Shape.h are present.
Install Adafruit_NeoPixel library before compiling.

Adjust PIXEL_BUFFER_SIZE and PIN to match your hardware.
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

// Matrix geometry
static const uint8_t W = 10;
static const uint8_t H = 20;

// Adafruit strip
Adafruit_NeoPixel strip(PIXEL_BUFFER_SIZE, PIN, NEO_GRB + NEO_KHZ800);

// PixelGrid + LCD panel
Pixel_Grid* pixelGrid;
LCD_Panel* lcdPanel;

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
  void latch() { prevStable = stable; }
};

Btn btnL, btnR, btnRot;

// Tetris state
uint8_t board[H][W];

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
  uint8_t type = 0;
  uint8_t rot  = 0;
  int8_t x     = 3;
  int8_t y     = 0;
};

Piece cur;
bool gameOver = false;
uint32_t score = 0;
static uint32_t tFall = 0;
static const uint16_t FALL_MS = 550;

// piece colors (Adafruit strip.Color values)
uint32_t PIECE_COLORS_U32[7];

static inline uint16_t shapeMask(uint8_t type, uint8_t rot) {
  return pgm_read_word(&SHAPES[type][rot & 3]);
}

static inline bool maskCell(uint16_t mask, uint8_t cx, uint8_t cy) {
  uint8_t bitIndex = (uint8_t)(15 - (cy * 4 + cx));
  return (mask >> bitIndex) & 1;
}

static bool validAt(int8_t nx, int8_t ny, uint8_t nrot) {
  uint16_t mask = shapeMask(cur.type, nrot);

  for (uint8_t cy = 0; cy < 4; cy++) {
    for (uint8_t cx = 0; cx < 4; cx++) {
      if (!maskCell(mask, cx, cy)) continue;

      int8_t bx = nx + (int8_t)cx;
      int8_t by = ny + (int8_t)cy;

      if (bx < 0 || bx >= (int8_t)W || by >= (int8_t)H) return false;
      if (by >= 0 && board[by][bx] != 0) return false;
    }
  }
  return true;
}

static void clearBoard() {
  for (uint8_t y = 0; y < H; y++)
    for (uint8_t x = 0; x < W; x++)
      board[y][x] = 0;
}

static void spawnPiece() {
  cur.type = (uint8_t)random(0, 7);
  cur.rot  = 0;
  cur.x    = 3;
  cur.y    = 0;

  if (!validAt(cur.x, cur.y, cur.rot)) {
    gameOver = true;
  }
}

static void lockPiece() {
  uint16_t mask = shapeMask(cur.type, cur.rot);

  for (uint8_t cy = 0; cy < 4; cy++) {
    for (uint8_t cx = 0; cx < 4; cx++) {
      if (!maskCell(mask, cx, cy)) continue;

      int8_t bx = cur.x + (int8_t)cx;
      int8_t by = cur.y + (int8_t)cy;
      if (by >= 0 && by < (int8_t)H && bx >= 0 && bx < (int8_t)W) {
        board[by][bx] = cur.type + 1;
      }
    }
  }
}

static uint8_t clearLines() {
  uint8_t lines = 0;

  for (int8_t y = (int8_t)H - 1; y >= 0; y--) {
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

static void debugPrintBoard() {
  Serial.println(F("board (logical rows top->bottom):"));
  for (uint8_t y = 0; y < H; ++y) {
    for (uint8_t x = 0; x < W; ++x) {
      Serial.print(board[y][x]);
      Serial.print(' ');
    }
    Serial.println();
  }
  Serial.println();
}

// Helper: write score into lcdPanel as characters (right-aligned)
static void updateScoreDigits(uint32_t s) {
  char tmp[7];
  char out[6];
  // right-aligned 6 chars (spaces for leading)
  snprintf(tmp, sizeof(tmp), "%6lu", (unsigned long)s);
  for (uint8_t i = 0; i < 6; ++i) out[i] = tmp[i];
  lcdPanel->changeCharArray(out);
  // digits are rendered in renderFrame with lcdPanel->render()
}

// Hard drop: move piece all the way down, lock, clear, spawn
static void hardDrop() {
  while (validAt(cur.x, cur.y + 1, cur.rot)) {
    cur.y++;
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
    updateScoreDigits(score);
  }
  spawnPiece();
  tFall = millis(); // reset fall timer
}

static void tryMove(int8_t dx, int8_t dy) {
  int8_t nx = cur.x + dx;
  int8_t ny = cur.y + dy;

  if (validAt(nx, ny, cur.rot)) {
    cur.x = nx;
    cur.y = ny;
    return;
  }

  // If down failed: lock -> clear -> score -> spawn
  if (dy == 1) {
    lockPiece();
    Serial.println(F("locked piece"));
    uint8_t cleared = clearLines();

    if (cleared > 0) {
      score += (uint32_t)cleared * 100UL; // +100 per row
      Serial.print(F("cleared: "));
      Serial.print(cleared);
      Serial.print(F("  score: "));
      Serial.println(score);
      debugPrintBoard();
      updateScoreDigits(score); // update the digit characters
    }
    spawnPiece();
  }
}

static void tryRotate() {
  uint8_t nr = (cur.rot + 1) & 3;

  const int8_t kicks[] = { 0, -1, 1, -2, 2 };
  for (uint8_t i = 0; i < sizeof(kicks); i++) {
    int8_t nx = cur.x + kicks[i];
    if (validAt(nx, cur.y, nr)) {
      cur.x = nx;
      cur.rot = nr;
      return;
    }
  }
}

static void renderFrame() {
  if (gameOver) {
    pixelGrid->setGridColour(strip.Color(60, 0, 0));
    lcdPanel->render();
    pixelGrid->render();
    strip.show();
    return;
  }

  // background
  pixelGrid->setGridColour(strip.Color(6, 6, 12));

  // locked blocks (flip row -> physical row)
  for (uint8_t y = 0; y < H; y++) {
    for (uint8_t x = 0; x < W; x++) {
      uint8_t v = board[y][x];
      if (v == 0) continue;
      pixelGrid->setGridCellColour((uint16_t)(H - 1 - y), x, PIECE_COLORS_U32[v - 1]);
    }
  }

  // current piece (flip row -> physical row)
  uint16_t mask = shapeMask(cur.type, cur.rot);
  for (uint8_t cy = 0; cy < 4; cy++) {
    for (uint8_t cx = 0; cx < 4; cx++) {
      if (!maskCell(mask, cx, cy)) continue;
      int8_t bx = cur.x + (int8_t)cx;
      int8_t by = cur.y + (int8_t)cy;
      if (by >= 0 && bx >= 0 && bx < W && by < H) {
        pixelGrid->setGridCellColour((uint16_t)(H - 1 - by), bx, PIECE_COLORS_U32[cur.type]);
      }
    }
  }

  // digits (score): lcdPanel already has char array set by updateScoreDigits
  lcdPanel->render();

  // render buffers into strip and show once
  pixelGrid->render();
  strip.show();
}

static void resetGame() {
  clearBoard();
  gameOver = false;
  score = 0;
  spawnPiece();
  tFall = millis();
  updateScoreDigits(score); // show 0 on digits
}

void setup() {
  randomSeed(analogRead(A0));
  Serial.begin(115200);   // enable debug output

  // strip & classes
  strip.begin();
  strip.show(); // clear

  pixelGrid = new Pixel_Grid(&strip, 0, H, W);      // start index 0 for matrix
  lcdPanel  = new LCD_Panel(&strip, 214, 6, strip.Color(255,255,255)); // digits start at 214

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

  // startup: show zeros (instead of '8's)
  updateScoreDigits(0);
  lcdPanel->render();
  pixelGrid->setGridColour(strip.Color(6,6,12));
  pixelGrid->render();
  strip.show();

  resetGame();
}

void loop() {
  // update buttons
  btnL.update();
  btnR.update();
  btnRot.update();

  if (gameOver) {
    if (btnRot.pressedEdge()) resetGame();
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

  if ((L_edge && R_down) || (R_edge && L_down) || (L_edge && R_edge)) {
    hardDrop();
    // consume the edges so we don't re-trigger while holding
    btnL.latch();
    btnR.latch();
  } else {
    if (L_edge) tryMove(-1, 0);
    if (R_edge) tryMove( 1, 0);
    if (btnRot.pressedEdge()) tryRotate();
  }

  uint32_t now = millis();
  if (now - tFall >= FALL_MS) {
    tFall = now;
    tryMove(0, 1);
  }

  renderFrame();

  btnL.latch(); btnR.latch(); btnRot.latch();

  delay(10);
}