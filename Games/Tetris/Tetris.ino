#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <PixelGridCore.h>
#include "HostRuntime.h"
#include "Pins.h"
#include "Input.h"
#include "Render.h"
#include "Game.h"

#include "NetSubmit.h" // WiFi + score submit

// Hardware objects
Adafruit_NeoPixel strip(PIXEL_BUFFER_SIZE, PIN_LED, NEO_GRB + NEO_KHZ800);
Pixel_Grid* pixelGrid = nullptr;
LCD_Panel* lcdPanel = nullptr;

Renderer renderer;
Input input;
TetrisGame game;

enum AppState {
  STATE_TITLE,
  STATE_PLAYING,
  STATE_GAMEOVER_HOLD
};

AppState state = STATE_TITLE;

// Title scroll control (right-to-left)
static const uint16_t TITLE_STEP_MS = 120;
static const int16_t  TITLE_TEXT_WIDTH = 6 * 6; // 36px
int16_t titleX = W;
uint32_t tTitle = 0;

// submit latch: ensure we submit once per game over
static bool submittedThisGame = false;

static inline bool anyStartButtonPressed(const InputState& s) {
  return s.anyButtonPressed;
}

static void enterTitle() {
  state = STATE_TITLE;
  titleX = W;
  tTitle = millis();
  input.resetRepeatTimers(millis());
}

static void enterPlaying() {
  state = STATE_PLAYING;
  submittedThisGame = false;
  game.reset(renderer);
  input.resetRepeatTimers(millis());
}

static void enterGameOverHold() {
  state = STATE_GAMEOVER_HOLD;
}

void initStandaloneMode() {
  randomSeed(analogRead(A0));

  strip.begin();
  strip.show();

  if (!pixelGrid) pixelGrid = new Pixel_Grid(&strip, 0, MATRIX_ROWS, W);
  if (!lcdPanel)  lcdPanel  = new LCD_Panel(&strip, 214, 6, strip.Color(255, 255, 255));

  renderer.begin(&strip, pixelGrid, lcdPanel);
  input.begin();

  game.initColours(renderer);
  renderer.setScoreDigits(0);

  submittedThisGame = false;
  enterTitle();
}

void runStandaloneLoop() {
  uint32_t now = millis();

  input.update();
  InputState in = input.sampleEdgesOnly();

  if (state == STATE_TITLE) {
    if (now - tTitle >= TITLE_STEP_MS) {
      tTitle = now;
      titleX -= 1;
      if (titleX < -TITLE_TEXT_WIDTH) titleX = W;
    }

    renderer.drawTitleScroll_TETRIS(titleX);

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

      if (!submittedThisGame) {
        submittedThisGame = true;

        String code;
        if (submitScoreToServer(game.score, code)) {
          renderer.setDigitsText(code.c_str()); // show 6-digit code
        } else {
          renderer.setDigitsText("NoNet");
        }
      }

      input.latch();
      return;
    }

    game.render(renderer);
  }
  else if (state == STATE_GAMEOVER_HOLD) {
    renderer.drawGameOverHold();

    if (anyStartButtonPressed(in)) {
      enterTitle();
      input.latch();
      return;
    }
  }

  input.latch();
}

// -----------------------------
// Host/standalone runtime setup
// -----------------------------
void setup()
{
  randomSeed(analogRead(A0));
  Serial.begin(115200);
  Serial.setTimeout(10);

  strip.begin();
  strip.show();

  pixelGrid = new Pixel_Grid(&strip, 0, MATRIX_ROWS, W);
  lcdPanel  = new LCD_Panel(&strip, 214, 6, strip.Color(255, 255, 255));

  resetHostParser();
  initStandaloneMode();
}

// -----------------------------
// Host/standalone runtime loop
// -----------------------------
void loop()
{
  bool gotHostFrame = tryReadHostFrame();

  if (runtimeMode == MODE_HOST) {
    if (millis() - lastHostFrameMs > HOST_TIMEOUT_MS) {
      runtimeMode = MODE_STANDALONE;
      resetHostParser();
      initStandaloneMode();
      return;
    }

    if (gotHostFrame || hostHasFrame) {
      renderHostFrame();
    } else {
      renderer.setDigitsText("HOST  ");
      renderer.show();
    }
    return;
  }

  runStandaloneLoop();
}
static void renderHostFrame() {
  if (!hostHasFrame) return;

  int idx = 0;

  for (uint8_t x = 0; x < W; ++x) {
    bool reverseCol = (x % 2 == 1);

    for (uint8_t i = 0; i < MATRIX_ROWS; ++i) {
      uint8_t row = reverseCol ? (uint8_t)(MATRIX_ROWS - 1 - i) : i;

      // hostGrb is GRB
      uint8_t g = hostGrb[idx + 0];
      uint8_t r = hostGrb[idx + 1];
      uint8_t b = hostGrb[idx + 2];

      // Convert to NeoPixel packed color
      uint32_t c = strip.Color(r, g, b);

      uint16_t pixelRow = (uint16_t)(MATRIX_ROWS - 1 - row);
      pixelGrid->setGridCellColour(pixelRow, x, c);

      idx += 3;
      if (idx + 2 >= (int)sizeof(hostGrb)) break;
    }
  }

  renderer.show();
}