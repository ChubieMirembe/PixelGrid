/*
Breakout for PixelGrid (Adafruit_NeoPixel + PixelGridCore)

Assumptions (matching your Tetris wiring/layout):
- 10 x 20 matrix (W=10, MATRIX_ROWS=20)
- Buttons:
    LEFT  on PIN 3
    ROT   on PIN 4   (used as "launch / serve" and "restart")
    RIGHT on PIN 5
- LED strip total buffer: 300 (matrix + digits area etc.)
- 6-digit panel starts at LED index 214 (same as your Tetris)

Gameplay:
- Bricks at the top of the play area (below PREVIEW_ROWS)
- Paddle at the bottom row
- Ball bounces; miss = lose a life
- ROT button serves ball when stuck on paddle
- Score displayed on 6-digit panel

No file copying required as long as PixelGridCore lives in your repo libraries folder
and Arduino IDE Sketchbook location points to the repo root.
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

Btn btnL, btnR, btnAct;

// ---------- Colors ----------
uint32_t PREVIEW_BG_COLOR_U32;
uint32_t PLAY_BG_COLOR_U32;

uint32_t BRICK_COLORS_U32[4];
uint32_t PADDLE_COLOR_U32;
uint32_t BALL_COLOR_U32;
uint32_t WALL_COLOR_U32;
uint32_t LIFE_COLOR_U32;

static inline uint8_t colR(uint32_t c) { return (uint8_t)((c >> 16) & 0xFF); }
static inline uint8_t colG(uint32_t c) { return (uint8_t)((c >> 8) & 0xFF); }
static inline uint8_t colB(uint32_t c) { return (uint8_t)(c & 0xFF); }

static inline uint32_t dimColor(uint32_t color, uint8_t percent) {
  uint8_t r = (uint8_t)((uint16_t)colR(color) * percent / 100);
  uint8_t g = (uint8_t)((uint16_t)colG(color) * percent / 100);
  uint8_t b = (uint8_t)((uint16_t)colB(color) * percent / 100);
  return strip.Color(r, g, b);
}

// Map logical play row (0..PLAY_H-1) to PixelGrid row index.
static inline uint16_t playRowToPixelRow(uint8_t logicalRow) {
  return (uint16_t)(MATRIX_ROWS - 1 - (PREVIEW_ROWS + logicalRow));
}
static inline uint16_t previewRowToPixelRow(uint8_t p) {
  return (uint16_t)(MATRIX_ROWS - 1 - p);
}

// ---------- Breakout state ----------
static const uint8_t BRICK_ROWS = 5;          // rows of bricks in play space
static const uint8_t BRICK_COLS = W;          // one brick per column
static const uint8_t BRICK_TOP_ROW = 0;       // starts at play row 0
static const uint8_t PADDLE_Y = PLAY_H - 1;   // bottom play row
static const uint8_t PADDLE_W = 3;            // paddle width in cells

static const uint16_t FRAME_MS = 16;          // ~60fps
static const uint16_t BALL_STEP_MS = 110;     // ball speed (lower = faster)
static const uint16_t BALL_STEP_MIN_MS = 55;
static const uint16_t BALL_SPEEDUP_EVERY = 8; // speed up after this many bricks
static const uint8_t  WALLS_ENABLED = 1;      // draw side walls in render

// bricks[y][x] = 0 empty, 1..4 = brick color tier
uint8_t bricks[BRICK_ROWS][BRICK_COLS];

int8_t paddleX = 3;        // leftmost x of paddle
bool ballStuck = true;     // ball on paddle waiting for serve
int8_t ballX = 0;
int8_t ballY = 0;
int8_t ballVX = 1;         // -1 or +1
int8_t ballVY = -1;        // -1 up, +1 down

uint8_t lives = 3;
uint32_t score = 0;
bool gameOver = false;

uint16_t ballStepMs = BALL_STEP_MS;
uint32_t tBall = 0;
uint16_t bricksHit = 0;

// ---------- Digits ----------
static void updateScoreDigits(uint32_t s) {
  char tmp[7];
  char out[6];
  snprintf(tmp, sizeof(tmp), "%6lu", (unsigned long)s);
  for (uint8_t i = 0; i < 6; ++i) out[i] = tmp[i];
  lcdPanel->changeCharArray(out);
}

// ---------- Game helpers ----------
static void clearBricks() {
  for (uint8_t y = 0; y < BRICK_ROWS; ++y) {
    for (uint8_t x = 0; x < BRICK_COLS; ++x) {
      bricks[y][x] = 0;
    }
  }
}

static void buildLevel() {
  clearBricks();
  // Simple gradient tiers from top to bottom
  // Top row hardest/brightest, etc.
  for (uint8_t y = 0; y < BRICK_ROWS; ++y) {
    uint8_t tier = 1;
    if (y == 0) tier = 4;
    else if (y == 1) tier = 3;
    else if (y == 2) tier = 2;
    else tier = 1;

    for (uint8_t x = 0; x < BRICK_COLS; ++x) {
      bricks[y][x] = tier;
    }
  }
}

static void resetBallOnPaddle() {
  ballStuck = true;
  ballVX = (random(0, 2) == 0) ? -1 : 1;
  ballVY = -1;

  // Center ball above paddle
  ballX = (int8_t)(paddleX + (PADDLE_W / 2));
  ballY = (int8_t)(PADDLE_Y - 1);

  tBall = millis();
}

static void resetGame() {
  score = 0;
  lives = 3;
  gameOver = false;

  paddleX = (W - PADDLE_W) / 2;
  buildLevel();

  ballStepMs = BALL_STEP_MS;
  bricksHit = 0;

  updateScoreDigits(score);
  resetBallOnPaddle();
}

static bool isPaddleCell(int8_t x, int8_t y) {
  if (y != (int8_t)PADDLE_Y) return false;
  if (x < paddleX) return false;
  if (x >= paddleX + (int8_t)PADDLE_W) return false;
  return true;
}

static void clampPaddle() {
  if (paddleX < 0) paddleX = 0;
  if (paddleX > (int8_t)(W - PADDLE_W)) paddleX = (int8_t)(W - PADDLE_W);
}

static void movePaddle(int8_t dx) {
  paddleX += dx;
  clampPaddle();
  if (ballStuck) {
    ballX = (int8_t)(paddleX + (PADDLE_W / 2));
    ballY = (int8_t)(PADDLE_Y - 1);
  }
}

static void serveBall() {
  if (!ballStuck) return;
  ballStuck = false;
  tBall = millis();
}

// return true if brick existed and was removed
static bool hitBrickAt(int8_t x, int8_t y) {
  if (x < 0 || x >= (int8_t)W) return false;
  if (y < (int8_t)BRICK_TOP_ROW || y >= (int8_t)(BRICK_TOP_ROW + BRICK_ROWS)) return false;

  uint8_t by = (uint8_t)(y - BRICK_TOP_ROW);
  uint8_t bx = (uint8_t)x;

  uint8_t v = bricks[by][bx];
  if (v == 0) return false;

  bricks[by][bx] = 0;
  score += 10UL * (uint32_t)v;   // tier-based scoring
  bricksHit++;

  if (bricksHit % BALL_SPEEDUP_EVERY == 0) {
    if (ballStepMs > BALL_STEP_MIN_MS) ballStepMs = (uint16_t)max((int)BALL_STEP_MIN_MS, (int)ballStepMs - 8);
  }

  updateScoreDigits(score);
  return true;
}

static bool anyBricksLeft() {
  for (uint8_t y = 0; y < BRICK_ROWS; ++y) {
    for (uint8_t x = 0; x < BRICK_COLS; ++x) {
      if (bricks[y][x] != 0) return true;
    }
  }
  return false;
}

static void nextLevelOrWin() {
  if (anyBricksLeft()) return;

  // Level clear bonus
  score += 250;
  updateScoreDigits(score);

  // Rebuild level and speed up a bit
  buildLevel();
  if (ballStepMs > BALL_STEP_MIN_MS) ballStepMs = (uint16_t)max((int)BALL_STEP_MIN_MS, (int)ballStepMs - 12);

  resetBallOnPaddle();
}

// Core ball step with simple collision
static void stepBallOnce() {
  if (ballStuck) return;

  int8_t nx = (int8_t)(ballX + ballVX);
  int8_t ny = (int8_t)(ballY + ballVY);

  // Side walls
  if (nx < 0) {
    nx = 0;
    ballVX = 1;
  } else if (nx >= (int8_t)W) {
    nx = (int8_t)(W - 1);
    ballVX = -1;
  }

  // Top of play area (ny < 0)
  if (ny < 0) {
    ny = 0;
    ballVY = 1;
  }

  // Brick collision check
  // We do a simple "try move, if brick hit invert Y; if still brick maybe invert X too" approach.
  bool brickHit = false;
  if (hitBrickAt(nx, ny)) {
    brickHit = true;
    ballVY = (int8_t)(-ballVY);
    ny = (int8_t)(ballY + ballVY);
  } else {
    // Try hitting brick on X-only or Y-only to reduce tunneling
    if (hitBrickAt(nx, ballY)) {
      brickHit = true;
      ballVX = (int8_t)(-ballVX);
      nx = (int8_t)(ballX + ballVX);
    } else if (hitBrickAt(ballX, ny)) {
      brickHit = true;
      ballVY = (int8_t)(-ballVY);
      ny = (int8_t)(ballY + ballVY);
    }
  }

  if (brickHit) {
    nextLevelOrWin();
  }

  // Paddle collision
  if (ny == (int8_t)(PADDLE_Y) && nx >= paddleX && nx < paddleX + (int8_t)PADDLE_W && ballVY > 0) {
    // Bounce up
    ballVY = -1;
    ny = (int8_t)(PADDLE_Y - 1);

    // Angle control based on where it hit the paddle
    int8_t center = (int8_t)(paddleX + (PADDLE_W / 2));
    if (nx < center) ballVX = -1;
    else if (nx > center) ballVX = 1;
    else {
      // center hit: keep vx or randomize if zero-ish behavior
      if (ballVX == 0) ballVX = (random(0, 2) == 0) ? -1 : 1;
    }
  }

  // Bottom (miss) -> lose life
  if (ny > (int8_t)PADDLE_Y) {
    if (lives > 0) lives--;
    if (lives == 0) {
      gameOver = true;
    } else {
      resetBallOnPaddle();
    }
    return;
  }

  ballX = nx;
  ballY = ny;
}

// ---------- Rendering ----------
static void drawBackground() {
  for (uint8_t p = 0; p < PREVIEW_ROWS; ++p) {
    uint16_t pixelRow = previewRowToPixelRow(p);
    for (uint8_t col = 0; col < W; ++col) {
      pixelGrid->setGridCellColour(pixelRow, col, PREVIEW_BG_COLOR_U32);
    }
  }

  for (uint8_t pr = 0; pr < PLAY_H; ++pr) {
    uint16_t pixelRow = playRowToPixelRow(pr);
    for (uint8_t col = 0; col < W; ++col) {
      pixelGrid->setGridCellColour(pixelRow, col, PLAY_BG_COLOR_U32);
    }
  }
}

static void drawLivesInPreview() {
  // Draw lives as small blocks on the left of preview row 0
  // Each life uses one cell; max shown up to 5 (safe for W=10)
  uint8_t shown = (lives > 5) ? 5 : lives;
  for (uint8_t i = 0; i < shown; ++i) {
    pixelGrid->setGridCellColour(previewRowToPixelRow(0), i, LIFE_COLOR_U32);
  }

  // Also show "ball stuck" indicator on preview row 1, right side
  if (ballStuck && !gameOver) {
    for (uint8_t x = W - 3; x < W; ++x) {
      pixelGrid->setGridCellColour(previewRowToPixelRow(1), x, dimColor(BALL_COLOR_U32, 35));
    }
  }
}

static void drawBricks() {
  for (uint8_t y = 0; y < BRICK_ROWS; ++y) {
    uint8_t py = (uint8_t)(BRICK_TOP_ROW + y);
    uint16_t pixelRow = playRowToPixelRow(py);
    for (uint8_t x = 0; x < BRICK_COLS; ++x) {
      uint8_t v = bricks[y][x];
      if (v == 0) continue;
      uint8_t tier = (v > 4) ? 4 : v;
      pixelGrid->setGridCellColour(pixelRow, x, BRICK_COLORS_U32[tier - 1]);
    }
  }
}

static void drawPaddle() {
  uint16_t pixelRow = playRowToPixelRow(PADDLE_Y);
  for (uint8_t x = 0; x < PADDLE_W; ++x) {
    uint8_t px = (uint8_t)(paddleX + x);
    if (px < W) pixelGrid->setGridCellColour(pixelRow, px, PADDLE_COLOR_U32);
  }
}

static void drawBall() {
  if (gameOver) return;
  if (ballX < 0 || ballX >= (int8_t)W) return;
  if (ballY < 0 || ballY >= (int8_t)PLAY_H) return;

  uint16_t pixelRow = playRowToPixelRow((uint8_t)ballY);
  pixelGrid->setGridCellColour(pixelRow, (uint16_t)ballX, BALL_COLOR_U32);
}

static void drawWalls() {
  if (!WALLS_ENABLED) return;

  // Side walls: draw a subtle vertical border in play area
  uint32_t c = dimColor(WALL_COLOR_U32, 35);
  for (uint8_t pr = 0; pr < PLAY_H; ++pr) {
    uint16_t r = playRowToPixelRow(pr);
    pixelGrid->setGridCellColour(r, 0, c);
    pixelGrid->setGridCellColour(r, W - 1, c);
  }
}

static void finalizeDigitsAndShow() {
  lcdPanel->render();
  pixelGrid->render();
  strip.show();
}

static void renderFrame() {
  if (gameOver) {
    // Red-tint screen on game over
    uint32_t c = strip.Color(30, 0, 0);
    for (uint8_t p = 0; p < PREVIEW_ROWS; ++p) {
      uint16_t r = previewRowToPixelRow(p);
      for (uint8_t x = 0; x < W; ++x) pixelGrid->setGridCellColour(r, x, c);
    }
    for (uint8_t pr = 0; pr < PLAY_H; ++pr) {
      uint16_t r = playRowToPixelRow(pr);
      for (uint8_t x = 0; x < W; ++x) pixelGrid->setGridCellColour(r, x, c);
    }
    finalizeDigitsAndShow();
    return;
  }

  drawBackground();
  drawLivesInPreview();
  drawWalls();
  drawBricks();
  drawPaddle();
  drawBall();
  finalizeDigitsAndShow();
}

// ---------- Arduino ----------
void setup() {
  randomSeed(analogRead(A0));
  Serial.begin(115200);

  strip.begin();
  strip.show();

  // Background colors
  PREVIEW_BG_COLOR_U32 = strip.Color(60, 60, 90);
  PLAY_BG_COLOR_U32    = strip.Color(6, 6, 12);

  // Game colors
  BRICK_COLORS_U32[0] = strip.Color(0, 170, 255);
  BRICK_COLORS_U32[1] = strip.Color(0, 220, 120);
  BRICK_COLORS_U32[2] = strip.Color(240, 210, 0);
  BRICK_COLORS_U32[3] = strip.Color(255, 90, 0);

  PADDLE_COLOR_U32 = strip.Color(220, 220, 220);
  BALL_COLOR_U32   = strip.Color(255, 255, 255);
  WALL_COLOR_U32   = strip.Color(200, 200, 255);
  LIFE_COLOR_U32   = strip.Color(255, 80, 80);

  pixelGrid = new Pixel_Grid(&strip, 0, MATRIX_ROWS, W);
  lcdPanel  = new LCD_Panel(&strip, 214, 6, strip.Color(255, 255, 255));

  btnL.begin(PIN_LEFT);
  btnR.begin(PIN_RIGHT);
  btnAct.begin(PIN_ROT);

  updateScoreDigits(0);
  lcdPanel->render();

  // Initial clear draw
  drawBackground();
  pixelGrid->render();
  strip.show();

  resetGame();
}

void loop() {
  static uint32_t tFrame = 0;
  uint32_t now = millis();
  if (now - tFrame < FRAME_MS) return;
  tFrame = now;

  btnL.update();
  btnR.update();
  btnAct.update();

  if (gameOver) {
    // ROT to restart
    if (btnAct.pressedEdge()) resetGame();
    renderFrame();
    btnL.latch();
    btnR.latch();
    btnAct.latch();
    return;
  }

  // Paddle movement (edge-based, but supports holding by repeating every few frames)
  // Simple hold repeat using stable state
  static uint8_t repeatCounter = 0;
  repeatCounter++;

  if (btnL.pressedEdge() || (btnL.stable && (repeatCounter % 3 == 0))) movePaddle(-1);
  if (btnR.pressedEdge() || (btnR.stable && (repeatCounter % 3 == 0))) movePaddle(1);

  // Serve ball
  if (btnAct.pressedEdge()) serveBall();

  // Ball update
  if (!ballStuck) {
    if (now - tBall >= ballStepMs) {
      tBall = now;
      stepBallOnce();
    }
  } else {
    // keep ball positioned on paddle
    ballX = (int8_t)(paddleX + (PADDLE_W / 2));
    ballY = (int8_t)(PADDLE_Y - 1);
  }

  renderFrame();

  btnL.latch();
  btnR.latch();
  btnAct.latch();
}
