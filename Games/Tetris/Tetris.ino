#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <PixelGridCore.h>

#include "Pins.h"
#include "Input.h"
#include "Render.h"
#include "Game.h"

// Hardware objects
Adafruit_NeoPixel strip(PIXEL_BUFFER_SIZE, PIN_LED, NEO_GRB + NEO_KHZ800);
Pixel_Grid* pixelGrid = nullptr;
LCD_Panel* lcdPanel = nullptr;

Renderer renderer;
Input input;
TetrisGame game;

enum AppState {
  STATE_TITLE,          // title scroll running, waits for button press to start game
  STATE_PLAYING,        // normal game
  STATE_GAMEOVER_HOLD   // solid red, waits for button press to go back to title
};

AppState state = STATE_TITLE;

// Title scroll control (right-to-left)
static const uint16_t TITLE_STEP_MS = 120;
static const int16_t  TITLE_TEXT_WIDTH = 6 * 6; // 6 letters, (5px + 1 gap) => 36px
int16_t titleX = W; // start off-screen to the right
uint32_t tTitle = 0;

static inline bool anyStartButtonPressed(const InputState& s) {
  // “start” is any of the 4 buttons (pins 3/4/9/10)
  return s.anyButtonPressed;
}

void enterTitle() {
  state = STATE_TITLE;
  titleX = W;             // restart scroll
  tTitle = millis();
  input.resetRepeatTimers(millis());
}

void enterPlaying() {
  state = STATE_PLAYING;
  game.reset(renderer);
  input.resetRepeatTimers(millis());
}

void enterGameOverHold() {
  state = STATE_GAMEOVER_HOLD;
}

void setup() {
  randomSeed(analogRead(A0));
  Serial.begin(115200);

  strip.begin();
  strip.show();

  pixelGrid = new Pixel_Grid(&strip, 0, MATRIX_ROWS, W);
  lcdPanel  = new LCD_Panel(&strip, 214, 6, strip.Color(255, 255, 255));

  renderer.begin(&strip, pixelGrid, lcdPanel);

  input.begin();

  // init game colours once we have renderer/strip
  game.initColours(renderer);

  renderer.setScoreDigits(0);

  // POWER ON behaviour: go straight into title scroll
  enterTitle();
}

void loop() {
  uint32_t now = millis();

  // update input
  input.update();
  InputState in = input.sampleEdgesOnly();

  if (state == STATE_TITLE) {
    // scroll text right-to-left
    if (now - tTitle >= TITLE_STEP_MS) {
      tTitle = now;
      titleX -= 1;
      if (titleX < -TITLE_TEXT_WIDTH) titleX = W; // loop
    }

    renderer.drawTitleScroll_TETRIS(titleX);

    // start game on button press
    if (anyStartButtonPressed(in)) {
      enterPlaying();
      input.latch();
      return;
    }
  }
  else if (state == STATE_PLAYING) {
    int8_t dx = input.joystickRepeatDx(now);
    game.update(in, dx, now, renderer);

    if (game.isGameOver()) {
      enterGameOverHold();
      input.latch();
      return;
    }

    game.render(renderer);
  }
  else if (state == STATE_GAMEOVER_HOLD) {
    renderer.drawGameOverHold();

    // stay red until button press
    if (anyStartButtonPressed(in)) {
      // first press after game over returns to title (and restarts scroll)
      enterTitle();
      input.latch();
      return;
    }
  }

  input.latch();
}
