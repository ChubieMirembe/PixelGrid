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

// Public (simple): keep these as globals so Game can read them easily
static Btn btn1, btn2, btn3, btn4;
static Btn joyU, joyL, joyR, joyD;

static inline void inputInit() {
  btn1.begin(PIN_BTN1);
  btn2.begin(PIN_BTN2);
  btn3.begin(PIN_BTN3);
  btn4.begin(PIN_BTN4);

  joyU.begin(PIN_JOY_UP);
  joyL.begin(PIN_JOY_LEFT);
  joyR.begin(PIN_JOY_RIGHT);
  joyD.begin(PIN_JOY_DOWN);
}

static inline void inputUpdate() {
  btn1.update(); btn2.update(); btn3.update(); btn4.update();
  joyU.update(); joyL.update(); joyR.update(); joyD.update();
}

static inline void inputLatch() {
  btn1.latch(); btn2.latch(); btn3.latch(); btn4.latch();
  joyU.latch(); joyL.latch(); joyR.latch(); joyD.latch();
}

// Convenience queries (so Game doesn’t care which physical control caused it)
static inline bool rotateLeftPressed()  { return btn1.pressedEdge() || btn3.pressedEdge(); }
static inline bool rotateRightPressed() { return btn2.pressedEdge() || btn4.pressedEdge(); }
static inline bool holdPressed()        { return joyU.pressedEdge(); }

static inline bool leftHeld()   { return joyL.stable; }
static inline bool rightHeld()  { return joyR.stable; }
static inline bool downHeld()   { return joyD.stable; }

static inline bool leftPressed()  { return joyL.pressedEdge(); }
static inline bool rightPressed() { return joyR.pressedEdge(); }
static inline bool anyRotatePressed() {
  return btn1.pressedEdge() || btn2.pressedEdge() || btn3.pressedEdge() || btn4.pressedEdge();
}
