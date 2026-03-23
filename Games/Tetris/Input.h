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

  bool pressedEdge() const  { return stable && !prevStable; }
  bool releasedEdge() const { return !stable && prevStable; }
  void latch() { prevStable = stable; }
};

struct InputState {
  // edges
  bool rotLeftPressed  = false;
  bool rotRightPressed = false;
  bool holdPressed     = false;

  bool anyButtonPressed = false;

  // held (for repeat / soft drop)
  bool leftHeld  = false;
  bool rightHeld = false;
  bool downHeld  = false;
};

struct Input {
  Btn btn1, btn2, btn3, btn4;
  Btn joyU, joyL, joyR, joyD;

  // repeat state
  uint32_t tMoveL = 0;
  uint32_t tMoveR = 0;
  bool moveLRepeating = false;
  bool moveRRepeating = false;

  void begin() {
    btn1.begin(PIN_BTN1);
    btn2.begin(PIN_BTN2);
    btn3.begin(PIN_BTN3);
    btn4.begin(PIN_BTN4);

    joyU.begin(PIN_JOY_UP);
    joyL.begin(PIN_JOY_LEFT);
    joyR.begin(PIN_JOY_RIGHT);
    joyD.begin(PIN_JOY_DOWN);

    uint32_t now = millis();
    tMoveL = tMoveR = now;
    moveLRepeating = moveRRepeating = false;
  }

  void update() {
    btn1.update(); btn2.update(); btn3.update(); btn4.update();
    joyU.update(); joyL.update(); joyR.update(); joyD.update();
  }

  InputState sampleEdgesOnly() const {
    InputState s;

    bool leftEdge  = (btn1.pressedEdge() || btn3.pressedEdge());
    bool rightEdge = (btn2.pressedEdge() || btn4.pressedEdge());

    s.rotLeftPressed  = leftEdge;
    s.rotRightPressed = rightEdge;
    s.holdPressed     = joyU.pressedEdge();

    s.anyButtonPressed = (btn1.pressedEdge() || btn2.pressedEdge() || btn3.pressedEdge() || btn4.pressedEdge());

    s.leftHeld  = joyL.stable;
    s.rightHeld = joyR.stable;
    s.downHeld  = joyD.stable;

    return s;
  }

  // Call once per loop after you’re done using edges
  void latch() {
    btn1.latch(); btn2.latch(); btn3.latch(); btn4.latch();
    joyU.latch(); joyL.latch(); joyR.latch(); joyD.latch();
  }

  // Returns dx movement from joystick repeat logic: -1, 0, or +1
  int8_t joystickRepeatDx(uint32_t now) {
    int8_t dx = 0;

    // LEFT
    if (joyL.stable) {
      if (joyL.pressedEdge()) {
        dx = -1;
        tMoveL = now;
        moveLRepeating = false;
      } else {
        uint16_t waitMs = moveLRepeating ? MOVE_REPEAT_MS : MOVE_REPEAT_START_MS;
        if (now - tMoveL >= waitMs) {
          dx = -1;
          tMoveL = now;
          moveLRepeating = true;
        }
      }
    } else if (joyL.releasedEdge()) {
      moveLRepeating = false;
    }

    // RIGHT (only if not moving left this tick)
    if (dx == 0) {
      if (joyR.stable) {
        if (joyR.pressedEdge()) {
          dx = +1;
          tMoveR = now;
          moveRRepeating = false;
        } else {
          uint16_t waitMs = moveRRepeating ? MOVE_REPEAT_MS : MOVE_REPEAT_START_MS;
          if (now - tMoveR >= waitMs) {
            dx = +1;
            tMoveR = now;
            moveRRepeating = true;
          }
        }
      } else if (joyR.releasedEdge()) {
        moveRRepeating = false;
      }
    }

    return dx;
  }

  void resetRepeatTimers(uint32_t now) {
    tMoveL = tMoveR = now;
    moveLRepeating = moveRRepeating = false;
  }
};
