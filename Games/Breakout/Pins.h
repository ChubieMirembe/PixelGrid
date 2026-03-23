// Pins.h

I cannot edit files directly in this Ask-only session, but this is the exact source update needed to make Breakout match the ESP32-S3 mapping you chose (option 1).

Update only this file:
Pins.h

Replace the hardware pin section with this:

#pragma once
#include <Arduino.h>

// =====================
// Hardware pinout
// =====================
#define PIXEL_BUFFER_SIZE 300

#define PIN_LED 5

// Buttons (serve/restart ONLY)
#define PIN_BTN1 3
#define PIN_BTN2 4
#define PIN_BTN3 10
#define PIN_BTN4 11

// Joystick (movement ONLY)
#define PIN_JOY_UP 6 // unused (still read)
#define PIN_JOY_LEFT 7 // move left (repeat)
#define PIN_JOY_RIGHT 8 // move right (repeat)
#define PIN_JOY_DOWN 9 // unused (still read)

// =====================
// Grid dimensions
// =====================
static const uint8_t PREVIEW_ROWS = 0;
static const uint8_t PLAY_H = 20;
static const uint8_t W = 10;
static const uint8_t MATRIX_ROWS = PREVIEW_ROWS + PLAY_H;

// =====================
// Timing
// =====================
static const uint16_t FRAME_MS = 16;

static const uint16_t BALL_STEP_MS = 110;
static const uint16_t BALL_STEP_MIN_MS = 55;
static const uint16_t BALL_SPEEDUP_EVERY = 10;

static const uint16_t BRICK_DROP_MS = 15000; // 15 seconds

// Joystick move repeat (DAS/ARR-like)
static const uint16_t MOVE_REPEAT_START_MS = 160;
static const uint16_t MOVE_REPEAT_MS = 65;

// Debounce
static const uint16_t DEBOUNCE_MS = 18;

// =====================
// Game layout
// =====================
static const uint8_t PADDLE_Y = PLAY_H - 1; // bottom row (19)
static const uint8_t PADDLE_W = 3;

// Bricks live in play rows 0..18 inclusive (row 19 is paddle)
static const uint8_t BRICK_TOP = 0;
static const uint8_t BRICK_BOTTOM = PLAY_H - 2;           // 18
static const uint8_t BRICK_H = (uint8_t)(BRICK_BOTTOM + 1); // 19 rows total for bricks space
static const uint8_t INITIAL_FILLED_ROWS = 8;

// RGB wheel stepping
static const uint8_t COLOR_STEP = 9;
