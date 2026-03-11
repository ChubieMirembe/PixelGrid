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

static uint8_t lastHostPayload = 0;
static unsigned long lastHostSendMs = 0;
static const unsigned long HOST_SEND_MIN_MS = 15; // throttle to avoid flooding
static uint8_t buildHostPayload(const Input &inp) {
  uint8_t p = 0;
  p |= (inp.btn1.stable ? (1u << 0) : 0);
  p |= (inp.btn2.stable ? (1u << 1) : 0);
  p |= (inp.btn3.stable ? (1u << 2) : 0);
  p |= (inp.joyU.stable ? (1u << 3) : 0);
  p |= (inp.joyD.stable ? (1u << 4) : 0);
  p |= (inp.joyL.stable ? (1u << 5) : 0);
  p |= (inp.joyR.stable ? (1u << 6) : 0);
  p |= (inp.btn4.stable ? (1u << 7) : 0);
  return p;
}

static bool readBytesWithTimeout(uint8_t* buf, size_t len, unsigned long timeoutMs = 50) {
  unsigned long start = millis();
  size_t got = 0;
  while (got < len && (millis() - start) < timeoutMs) {
    if (Serial.available()) {
      int b = Serial.read();
      if (b >= 0) buf[got++] = (uint8_t)b;
    } else {
      delay(1);
    }
  }
  return (got == len);
}

// Handle a PBLC packet AFTER you've consumed the 'P','B','L','C' magic.
// Packet format (PC side): 'P' 'B' 'L' 'C' + uint16_t length (little-endian) + payload bytes (UTF8)
static void HandlePblcPacket() {
  // read 2-byte length
  uint8_t lenBytes[2];
  if (!readBytesWithTimeout(lenBytes, 2, 50)) return;
  uint16_t payloadLen = (uint16_t)lenBytes[0] | ((uint16_t)lenBytes[1] << 8);

  // cap to safe buffer size
  const size_t MAX_LCD = 32;
  uint16_t toRead = payloadLen;
  if (toRead > MAX_LCD) toRead = MAX_LCD;

  char buf[MAX_LCD + 1];
  size_t idx = 0;
  unsigned long start = millis();
  while (idx < toRead && (millis() - start) < 250) {
    if (Serial.available()) {
      int c = Serial.read();
      if (c >= 0) buf[idx++] = (char)c;
    } else {
      delay(1);
    }
  }
  buf[idx] = '\0';

  // If payloadLen > toRead flush remaining bytes so parser stays in sync
  if (payloadLen > toRead) {
    uint16_t skip = payloadLen - toRead;
    unsigned long flushStart = millis();
    while (skip-- > 0 && (millis() - flushStart) < 200) {
      if (Serial.available()) Serial.read(); else delay(1);
    }
  }

  // Update your renderer / LCD. Use the same API you use elsewhere:
  renderer.setDigitsText(buf);
}
static void sendHostInputIfChanged(const Input &inp) {
  uint8_t payload = buildHostPayload(inp);
  unsigned long now = millis();
  if (payload != lastHostPayload && (now - lastHostSendMs) >= HOST_SEND_MIN_MS) {
    Serial.write('b');       // marker expected by PC
    Serial.write(payload);   // single payload byte
    lastHostPayload = payload;
    lastHostSendMs = now;
  }
}
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
      input.update();
      sendHostInputIfChanged(input);
      input.latch();
      if (millis() - lastHostFrameMs > HOST_TIMEOUT_MS) {
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