/*
Breakout for PixelGrid (Adafruit_NeoPixel + PixelGridCore)

Update:
- Only the FIRST 8 play rows are occupied by bricks at start (play rows 0..7 filled).
- Rows 8..16 start empty.
- Every 15 seconds:
    - bricks shift DOWN by 1 (within play rows 0..16)
    - a NEW row is generated at play row 0
- Each newly generated row uses ONE uniform colour across the row.
- Row colours cycle through the full 0..255 RGB wheel in fixed steps.
- No drawn wall edges.
- One miss = game over.
- ROT button = serve / restart.
*/

#define PIXEL_BUFFER_SIZE 300
#include <Adafruit_NeoPixel.h>
#include <avr/pgmspace.h>
#include <PixelGridCore.h>

#define PIN 6
#define PIN_LEFT  3
#define PIN_ROT   4
#define PIN_RIGHT 5

static const uint8_t PREVIEW_ROWS = 0;
static const uint8_t PLAY_H = 20;
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

uint32_t PADDLE_COLOR_U32;
uint32_t BALL_COLOR_U32;

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

// ---------- Breakout constants ----------
static const uint8_t PADDLE_Y = PLAY_H - 1;      // row 17
static const uint8_t PADDLE_W = 3;

static const uint16_t FRAME_MS = 16;
static const uint16_t BALL_STEP_MS = 110;
static const uint16_t BALL_STEP_MIN_MS = 55;
static const uint16_t BALL_SPEEDUP_EVERY = 10;

static const uint16_t BRICK_DROP_MS = 15000;     // 15 seconds

// Bricks live in play rows 0..16 inclusive (row 17 is paddle)
static const uint8_t BRICK_TOP = 0;
static const uint8_t BRICK_BOTTOM = PLAY_H - 2;  // 16
static const uint8_t BRICK_H = (uint8_t)(BRICK_BOTTOM + 1); // 17 rows total for bricks space

static const uint8_t INITIAL_FILLED_ROWS = 8;    // play rows 0..7 filled at start

// RGB wheel stepping
static const uint8_t COLOR_STEP = 9;

// ---------- Game state ----------
// per-cell brick colour, 0 = empty
uint32_t bricksGrid[BRICK_H][W];

int8_t paddleX = 3;
bool ballStuck = true;
int8_t ballX = 0;
int8_t ballY = 0;
int8_t ballVX = 1;
int8_t ballVY = -1;

uint32_t score = 0;
bool gameOver = false;

uint16_t ballStepMs = BALL_STEP_MS;
uint32_t tBall = 0;
uint32_t tBrickDrop = 0;
uint16_t bricksHit = 0;

uint8_t wheelPos = 0;

// ---------- Digits ----------
static void updateScoreDigits(uint32_t s) {
  char tmp[7];
  char out[6];
  snprintf(tmp, sizeof(tmp), "%6lu", (unsigned long)s);
  for (uint8_t i = 0; i < 6; ++i) out[i] = tmp[i];
  lcdPanel->changeCharArray(out);
}

// ---------- Colour wheel ----------
static uint32_t wheelColor(uint8_t pos) {
  pos = (uint8_t)(255 - pos);
  if (pos < 85) {
    return strip.Color((uint8_t)(255 - pos * 3), 0, (uint8_t)(pos * 3));
  }
  if (pos < 170) {
    pos = (uint8_t)(pos - 85);
    return strip.Color(0, (uint8_t)(pos * 3), (uint8_t)(255 - pos * 3));
  }
  pos = (uint8_t)(pos - 170);
  return strip.Color((uint8_t)(pos * 3), (uint8_t)(255 - pos * 3), 0);
}

// ---------- Brick helpers ----------
static void clearBricks() {
  for (uint8_t y = 0; y < BRICK_H; ++y) {
    for (uint8_t x = 0; x < W; ++x) bricksGrid[y][x] = 0;
  }
}

static void generateBrickRowAt(uint8_t row, uint32_t c) {
  if (row >= BRICK_H) return;
  for (uint8_t x = 0; x < W; ++x) bricksGrid[row][x] = c;
}

static void fillInitialBricks() {
  // Fill only rows 0..7; rows 8..16 remain empty.
  clearBricks();
  for (uint8_t y = 0; y < INITIAL_FILLED_ROWS; ++y) {
    uint32_t c = wheelColor(wheelPos);
    wheelPos = (uint8_t)(wheelPos + COLOR_STEP);
    generateBrickRowAt(y, c);
  }
}

// Every 15s: shift bricks down; new row at top.
// If bottom brick row already occupied, next shift would push into paddle lane -> game over.
static void brickDropTick() {
  for (uint8_t x = 0; x < W; ++x) {
    if (bricksGrid[BRICK_BOTTOM][x] != 0) {
      gameOver = true;
      return;
    }
  }

  for (int8_t y = (int8_t)BRICK_BOTTOM; y > (int8_t)BRICK_TOP; --y) {
    for (uint8_t x = 0; x < W; ++x) {
      bricksGrid[(uint8_t)y][x] = bricksGrid[(uint8_t)(y - 1)][x];
    }
  }

  uint32_t c = wheelColor(wheelPos);
  wheelPos = (uint8_t)(wheelPos + COLOR_STEP);
  generateBrickRowAt(BRICK_TOP, c);
}

static bool hitBrickAt(int8_t x, int8_t y) {
  if (x < 0 || x >= (int8_t)W) return false;
  if (y < (int8_t)BRICK_TOP || y > (int8_t)BRICK_BOTTOM) return false;

  uint32_t v = bricksGrid[(uint8_t)y][(uint8_t)x];
  if (v == 0) return false;

  bricksGrid[(uint8_t)y][(uint8_t)x] = 0;

  score += 10UL;
  bricksHit++;

  if (bricksHit % BALL_SPEEDUP_EVERY == 0) {
    if (ballStepMs > BALL_STEP_MIN_MS) {
      ballStepMs = (uint16_t)max((int)BALL_STEP_MIN_MS, (int)ballStepMs - 6);
    }
  }

  updateScoreDigits(score);
  return true;
}

// ---------- Ball / Paddle helpers ----------
static void resetBallOnPaddle() {
  ballStuck = true;
  ballVX = (random(0, 2) == 0) ? -1 : 1;
  ballVY = -1;

  ballX = (int8_t)(paddleX + (PADDLE_W / 2));
  ballY = (int8_t)(PADDLE_Y - 1);

  tBall = millis();
}

static void resetGame() {
  score = 0;
  gameOver = false;

  paddleX = (W - PADDLE_W) / 2;

  ballStepMs = BALL_STEP_MS;
  bricksHit = 0;

  wheelPos = 0;
  fillInitialBricks();

  updateScoreDigits(score);
  resetBallOnPaddle();

  tBrickDrop = millis();
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

static void stepBallOnce() {
  if (ballStuck) return;

  int8_t nx = (int8_t)(ballX + ballVX);
  int8_t ny = (int8_t)(ballY + ballVY);

  // side bounds bounce (no drawn walls)
  if (nx < 0) {
    nx = 0;
    ballVX = 1;
  } else if (nx >= (int8_t)W) {
    nx = (int8_t)(W - 1);
    ballVX = -1;
  }

  // top bounce
  if (ny < 0) {
    ny = 0;
    ballVY = 1;
  }

  // brick collision
  if (hitBrickAt(nx, ny)) {
    ballVY = (int8_t)(-ballVY);
    ny = (int8_t)(ballY + ballVY);
  } else {
    if (hitBrickAt(nx, ballY)) {
      ballVX = (int8_t)(-ballVX);
      nx = (int8_t)(ballX + ballVX);
    } else if (hitBrickAt(ballX, ny)) {
      ballVY = (int8_t)(-ballVY);
      ny = (int8_t)(ballY + ballVY);
    }
  }

  // paddle collision
  if (ny == (int8_t)PADDLE_Y && ballVY > 0) {
    if (nx >= paddleX && nx < paddleX + (int8_t)PADDLE_W) {
      ballVY = -1;
      ny = (int8_t)(PADDLE_Y - 1);

      int8_t center = (int8_t)(paddleX + (PADDLE_W / 2));
      if (nx < center) ballVX = -1;
      else if (nx > center) ballVX = 1;
      else {
        if (ballVX == 0) ballVX = (random(0, 2) == 0) ? -1 : 1;
      }
    }
  }

  // miss -> game over
  if (ny > (int8_t)PADDLE_Y) {
    gameOver = true;
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

static void drawPreviewIndicator() {
  if (ballStuck && !gameOver) {
    for (uint8_t x = W - 3; x < W; ++x) {
      pixelGrid->setGridCellColour(previewRowToPixelRow(1), x, dimColor(BALL_COLOR_U32, 35));
    }
  }
}

static void drawBricks() {
  for (uint8_t y = BRICK_TOP; y <= BRICK_BOTTOM; ++y) {
    uint16_t pixelRow = playRowToPixelRow(y);
    for (uint8_t x = 0; x < W; ++x) {
      uint32_t c = bricksGrid[y][x];
      if (c == 0) continue;
      pixelGrid->setGridCellColour(pixelRow, x, c);
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

static void finalizeDigitsAndShow() {
  lcdPanel->render();
  pixelGrid->render();
  strip.show();
}

static void renderFrame() {
  if (gameOver) {
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
  drawPreviewIndicator();
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

  PREVIEW_BG_COLOR_U32 = strip.Color(60, 60, 90);
  PLAY_BG_COLOR_U32    = strip.Color(6, 6, 12);

  PADDLE_COLOR_U32 = strip.Color(220, 220, 220);
  BALL_COLOR_U32   = strip.Color(255, 255, 255);

  pixelGrid = new Pixel_Grid(&strip, 0, MATRIX_ROWS, W);
  lcdPanel  = new LCD_Panel(&strip, 214, 6, strip.Color(255, 255, 255));

  btnL.begin(PIN_LEFT);
  btnR.begin(PIN_RIGHT);
  btnAct.begin(PIN_ROT);

  updateScoreDigits(0);
  lcdPanel->render();

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
    if (btnAct.pressedEdge()) resetGame();
    renderFrame();
    btnL.latch();
    btnR.latch();
    btnAct.latch();
    return;
  }

  // Bricks drop every 15 seconds
  if (now - tBrickDrop >= BRICK_DROP_MS) {
    tBrickDrop = now;
    brickDropTick();
    if (gameOver) {
      renderFrame();
      btnL.latch();
      btnR.latch();
      btnAct.latch();
      return;
    }
  }

  // Paddle movement with basic repeat
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
    ballX = (int8_t)(paddleX + (PADDLE_W / 2));
    ballY = (int8_t)(PADDLE_Y - 1);
  }

  renderFrame();

  btnL.latch();
  btnR.latch();
  btnAct.latch();
}
