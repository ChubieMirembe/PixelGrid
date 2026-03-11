#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <PixelGridCore.h>
#include "HostRuntime.h"
#include "Pins.h"
#include "Input.h"
#include "Render.h"
#include "Game.h"

#include "NetSubmit.h" // <-- add this

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

// NEW: ensure we submit once per standalone game
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
  submittedThisGame = false;     // NEW: reset submit latch for next run
  game.reset(renderer);
  input.resetRepeatTimers(millis());
}

static void enterGameOverHold() {
  state = STATE_GAMEOVER_HOLD;
}

// ------------------------------------------------------------
// Standalone mode hooks (called by your host/standalone runtime)
// ------------------------------------------------------------
void initStandaloneMode() {
  // If host code already initialized hardware earlier, this is still safe on ESP32.
  randomSeed(analogRead(A0));

  // Don't re-Serial.begin() if your host runtime already did; but safe if called once.
  // If this can be called multiple times, it's still generally ok.
  Serial.begin(115200);

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

    // Only in standalone; never in host mode
    if (runtimeMode == MODE_STANDALONE && !submittedThisGame) {
      submittedThisGame = true;

      String code;
      if (submitScoreToServer(game.score, code)) {
        // Display the one-time code the user will type on the website
        renderer.setDigitsText(code.c_str());
      } else {
        // Display a simple error indicator
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

  if (gotHostFrame)
  {
    return; // host is active; NO standalone code runs here
  }

  if (runtimeMode == MODE_HOST)
  {
    if (millis() - lastHostFrameMs > HOST_TIMEOUT_MS)
    {
      runtimeMode = MODE_STANDALONE;
      resetHostParser();
      initStandaloneMode();
    }
    else
    {
      delay(1);
      return;
    }
  }

  runStandaloneLoop(); // only reaches here when standalone
}