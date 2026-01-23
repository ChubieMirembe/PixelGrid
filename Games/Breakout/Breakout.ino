// Breakout.ino
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <PixelGridCore.h>

#include "Pins.h"
#include "Input.h"
#include "Game.h"
#include "Render.h"

void setup() {
  randomSeed(analogRead(A0));
  Serial.begin(115200);

  Render_begin();
  Input_begin();
  Game_reset();
}

void loop() {
  static uint32_t tFrame = 0;
  uint32_t now = millis();
  if (now - tFrame < FRAME_MS) return;
  tFrame = now;

  // Update inputs
  Input_update();

  // Any button serves or restarts (edge)
  bool servePressed = Input_servePressedEdge();

  if (Game_isOver()) {
    if (servePressed) Game_reset();
    Render_renderFrame();
    Input_latch();
    return;
  }

  // Bricks drop every 15 seconds
  if (now - tBrickDrop >= BRICK_DROP_MS) {
    tBrickDrop = now;
    Game_brickDropTick();
    if (Game_isOver()) {
      Render_renderFrame();
      Input_latch();
      return;
    }
  }

  // Joystick movement (left/right only, with repeat)
  int8_t step = Input_paddleStepFromJoystickRepeat(now);
  if (step != 0) Game_movePaddle(step);

  // Serve ball on any button press
  if (servePressed) Game_serveBall();

  // Ball update
  if (!ballStuck) {
    if (now - tBall >= ballStepMs) {
      tBall = now;
      Game_stepBallOnce();
    }
  } else {
    // Keep ball glued to paddle (same as original)
    ballX = (int8_t)(paddleX + (PADDLE_W / 2));
    ballY = (int8_t)(PADDLE_Y - 1);
  }

  Render_renderFrame();

  // Latch edges
  Input_latch();
}
