// Game.cpp
#include "Game.h"
#include "Render.h"

// State
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
  clearBricks();
  for (uint8_t y = 0; y < INITIAL_FILLED_ROWS; ++y) {
    uint32_t c = Render_wheelColor(wheelPos);
    wheelPos = (uint8_t)(wheelPos + COLOR_STEP);
    generateBrickRowAt(y, c);
  }
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

  Render_updateScoreDigits(score);
  return true;
}

// ---------- Ball / Paddle helpers ----------
static void clampPaddle() {
  if (paddleX < 0) paddleX = 0;
  if (paddleX > (int8_t)(W - PADDLE_W)) paddleX = (int8_t)(W - PADDLE_W);
}

static void resetBallOnPaddle() {
  ballStuck = true;
  ballVX = (random(0, 2) == 0) ? -1 : 1;
  ballVY = -1;

  ballX = (int8_t)(paddleX + (PADDLE_W / 2));
  ballY = (int8_t)(PADDLE_Y - 1);

  tBall = millis();
}

void Game_movePaddle(int8_t dx) {
  paddleX += dx;
  clampPaddle();
  if (ballStuck) {
    ballX = (int8_t)(paddleX + (PADDLE_W / 2));
    ballY = (int8_t)(PADDLE_Y - 1);
  }
}

void Game_serveBall() {
  if (!ballStuck) return;
  ballStuck = false;
  tBall = millis();
}

void Game_reset() {
  score = 0;
  gameOver = false;

  paddleX = (W - PADDLE_W) / 2;

  ballStepMs = BALL_STEP_MS;
  bricksHit = 0;

  wheelPos = 0;
  fillInitialBricks();

  Render_updateScoreDigits(score);
  resetBallOnPaddle();

  tBrickDrop = millis();
}

void Game_brickDropTick() {
  // If bottom brick row already occupied, next shift would push into paddle lane -> game over.
  for (uint8_t x = 0; x < W; ++x) {
    if (bricksGrid[BRICK_BOTTOM][x] != 0) {
      gameOver = true;
      return;
    }
  }

  // Shift down
  for (int8_t y = (int8_t)BRICK_BOTTOM; y > (int8_t)BRICK_TOP; --y) {
    for (uint8_t x = 0; x < W; ++x) {
      bricksGrid[(uint8_t)y][x] = bricksGrid[(uint8_t)(y - 1)][x];
    }
  }

  // New top row
  uint32_t c = Render_wheelColor(wheelPos);
  wheelPos = (uint8_t)(wheelPos + COLOR_STEP);
  generateBrickRowAt(BRICK_TOP, c);
}

void Game_stepBallOnce() {
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

bool Game_isOver() { return gameOver; }
