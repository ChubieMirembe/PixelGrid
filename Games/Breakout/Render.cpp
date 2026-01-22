// Render.cpp
#include "Render.h"
#include "Game.h"

// Hardware objects
Adafruit_NeoPixel strip(PIXEL_BUFFER_SIZE, PIN_LED, NEO_GRB + NEO_KHZ800);
Pixel_Grid* pixelGrid = nullptr;
LCD_Panel* lcdPanel = nullptr;

// Colours
uint32_t PLAY_BG_COLOR_U32;
uint32_t PADDLE_COLOR_U32;
uint32_t BALL_COLOR_U32;

// Map logical play row (0..PLAY_H-1) to PixelGrid row index.
static inline uint16_t playRowToPixelRow(uint8_t logicalRow) {
  return (uint16_t)(MATRIX_ROWS - 1 - (PREVIEW_ROWS + logicalRow));
}

void Render_updateScoreDigits(uint32_t s) {
  char tmp[7];
  char out[6];
  snprintf(tmp, sizeof(tmp), "%6lu", (unsigned long)s);
  for (uint8_t i = 0; i < 6; ++i) out[i] = tmp[i];
  lcdPanel->changeCharArray(out);
}

uint32_t Render_wheelColor(uint8_t pos) {
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

static void drawBackground() {
  for (uint8_t pr = 0; pr < PLAY_H; ++pr) {
    uint16_t pixelRow = playRowToPixelRow(pr);
    for (uint8_t col = 0; col < W; ++col) {
      pixelGrid->setGridCellColour(pixelRow, col, PLAY_BG_COLOR_U32);
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

void Render_renderFrame() {
  if (gameOver) {
    uint32_t c = strip.Color(30, 0, 0);
    for (uint8_t pr = 0; pr < PLAY_H; ++pr) {
      uint16_t r = playRowToPixelRow(pr);
      for (uint8_t x = 0; x < W; ++x) pixelGrid->setGridCellColour(r, x, c);
    }
    finalizeDigitsAndShow();
    return;
  }

  drawBackground();
  drawBricks();
  drawPaddle();
  drawBall();
  finalizeDigitsAndShow();
}

void Render_begin() {
  strip.begin();
  strip.show();

  PLAY_BG_COLOR_U32 = strip.Color(6, 6, 12);
  PADDLE_COLOR_U32  = strip.Color(220, 220, 220);
  BALL_COLOR_U32    = strip.Color(255, 255, 255);

  pixelGrid = new Pixel_Grid(&strip, 0, MATRIX_ROWS, W);
  lcdPanel  = new LCD_Panel(&strip, 214, 6, strip.Color(255, 255, 255));

  Render_updateScoreDigits(0);
  lcdPanel->render();

  // Clear once
  for (uint8_t pr = 0; pr < PLAY_H; ++pr) {
    uint16_t r = playRowToPixelRow(pr);
    for (uint8_t x = 0; x < W; ++x) pixelGrid->setGridCellColour(r, x, PLAY_BG_COLOR_U32);
  }
  pixelGrid->render();
  strip.show();
}
