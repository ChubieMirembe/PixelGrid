#pragma once
#include <Arduino.h>

// ===== Hardware =====
#define PIXEL_BUFFER_SIZE 300
#define PIN_LED 2

// Buttons (rotate left/right in your Tetris)
#define PIN_BTN1 3
#define PIN_BTN2 4
#define PIN_BTN3 9
#define PIN_BTN4 10

// Joystick directions (digital)
#define PIN_JOY_UP    5   // hold
#define PIN_JOY_LEFT  6   // move left (repeat)
#define PIN_JOY_RIGHT 7   // move right (repeat)
#define PIN_JOY_DOWN  8   // soft drop while held

// ===== Grid =====
static const uint8_t PREVIEW_ROWS = 2;
static const uint8_t PLAY_H = 18;
static const uint8_t W = 10;
static const uint8_t MATRIX_ROWS = PREVIEW_ROWS + PLAY_H;

// ===== Timing =====
static const uint16_t DEBOUNCE_MS = 18;

// Joystick repeat tuning (DAS/ARR-ish)
static const uint16_t MOVE_REPEAT_START_MS = 160;
static const uint16_t MOVE_REPEAT_MS       = 65;
