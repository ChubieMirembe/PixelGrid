// Input.h
#pragma once
#include <Arduino.h>
#include "Pins.h"

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

// Buttons / joystick instances (global)
extern Btn btn1, btn2, btn3, btn4;
extern Btn joyL, joyR, joyU_unused, joyD_unused;

// Repeat state (global)
extern uint32_t tMoveL;
extern uint32_t tMoveR;
extern bool moveLRepeating;
extern bool moveRRepeating;

void Input_begin();
void Input_update();
void Input_latch();

// True on this frame if ANY serve/restart button was pressed (edge)
bool Input_servePressedEdge();

// Returns -1, 0, +1 based on joystick repeat logic (left/right only).
// Call once per frame; if non-zero, apply one paddle step.
int8_t Input_paddleStepFromJoystickRepeat(uint32_t now);
