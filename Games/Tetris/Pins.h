// Pins.h
#pragma once

// -------- Hardware --------
#define PIXEL_BUFFER_SIZE 300
#define PIN_LED 2

// Buttons
#define PIN_BTN1 3   // rotate left
#define PIN_BTN2 4   // rotate right
#define PIN_BTN3 9   // rotate left
#define PIN_BTN4 10  // rotate right

// Joystick directions (digital)
#define PIN_JOY_UP    5   // hold
#define PIN_JOY_LEFT  6   // move left (repeat)
#define PIN_JOY_RIGHT 7   // move right (repeat)
#define PIN_JOY_DOWN  8   // soft drop while held

// -------- Display layout --------
static const uint8_t PREVIEW_ROWS = 2;
static const uint8_t PLAY_H = 18;
static const uint8_t W = 10;
static const uint8_t MATRIX_ROWS = PREVIEW_ROWS + PLAY_H;

// -------- Input debounce --------
static const uint16_t DEBOUNCE_MS = 18;

// -------- Gravity / leveling --------
static const uint16_t BASE_FALL_MS = 550;
static const uint8_t  LINES_PER_LEVEL = 10;
static const uint16_t FALL_DECREMENT = 40;
static const uint16_t MIN_FALL_MS = 80;

// -------- Soft drop tuning --------
static const uint16_t SOFT_DROP_MIN_MS = 55;   // fastest while held
static const uint16_t SOFT_DROP_DIVISOR = 4;   // fallDelayMs / divisor (clamped)

// -------- Move auto-repeat (DAS/ARR-like) --------
static const uint16_t MOVE_REPEAT_START_MS = 160; // delay before repeating
static const uint16_t MOVE_REPEAT_MS = 65;        // repeat interval once repeating
