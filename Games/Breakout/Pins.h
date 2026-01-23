// Pins.h
#pragma once
#include <Arduino.h>

// =====================
// Hardware pinout
// =====================
#define PIXEL_BUFFER_SIZE 300

#define PIN_LED 2

// Buttons (serve/restart ONLY)
#define PIN_BTN1 3
#define PIN_BTN2 4
#define PIN_BTN3 9
#define PIN_BTN4 10

// Joystick (movement ONLY)
#define PIN_JOY_UP    5   // unused (still read)
#define PIN_JOY_LEFT  6   // move left (repeat)
#define PIN_JOY_RIGHT 7   // move right (repeat)
#define PIN_JOY_DOWN  8   // unused (still read)

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
