// Input.cpp
#include "Input.h"

// Instances
Btn btn1, btn2, btn3, btn4;
Btn joyL, joyR, joyU_unused, joyD_unused;

// Repeat state
uint32_t tMoveL = 0;
uint32_t tMoveR = 0;
bool moveLRepeating = false;
bool moveRRepeating = false;

void Input_begin() {
  // Buttons: serve/restart
  btn1.begin(PIN_BTN1);
  btn2.begin(PIN_BTN2);
  btn3.begin(PIN_BTN3);
  btn4.begin(PIN_BTN4);

  // Joystick: movement only (UP/DOWN unused but read)
  joyU_unused.begin(PIN_JOY_UP);
  joyL.begin(PIN_JOY_LEFT);
  joyR.begin(PIN_JOY_RIGHT);
  joyD_unused.begin(PIN_JOY_DOWN);

  uint32_t now = millis();
  tMoveL = tMoveR = now;
  moveLRepeating = moveRRepeating = false;
}

void Input_update() {
  btn1.update(); btn2.update(); btn3.update(); btn4.update();
  joyL.update(); joyR.update();
  joyU_unused.update(); joyD_unused.update();
}

void Input_latch() {
  btn1.latch(); btn2.latch(); btn3.latch(); btn4.latch();
  joyL.latch(); joyR.latch(); joyU_unused.latch(); joyD_unused.latch();
}

bool Input_servePressedEdge() {
  return btn1.pressedEdge() || btn2.pressedEdge() || btn3.pressedEdge() || btn4.pressedEdge();
}

int8_t Input_paddleStepFromJoystickRepeat(uint32_t now) {
  // LEFT
  if (joyL.stable) {
    if (joyL.pressedEdge()) {
      tMoveL = now;
      moveLRepeating = false;
      return -1;
    } else {
      uint16_t waitMs = moveLRepeating ? MOVE_REPEAT_MS : MOVE_REPEAT_START_MS;
      if (now - tMoveL >= waitMs) {
        tMoveL = now;
        moveLRepeating = true;
        return -1;
      }
    }
  } else if (joyL.releasedEdge()) {
    moveLRepeating = false;
  }

  // RIGHT
  if (joyR.stable) {
    if (joyR.pressedEdge()) {
      tMoveR = now;
      moveRRepeating = false;
      return +1;
    } else {
      uint16_t waitMs = moveRRepeating ? MOVE_REPEAT_MS : MOVE_REPEAT_START_MS;
      if (now - tMoveR >= waitMs) {
        tMoveR = now;
        moveRRepeating = true;
        return +1;
      }
    }
  } else if (joyR.releasedEdge()) {
    moveRRepeating = false;
  }

  return 0;
}
