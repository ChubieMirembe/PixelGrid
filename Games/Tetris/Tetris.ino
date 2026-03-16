#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <PixelGridCore.h>
#include "HostRuntime.h"
#include "Pins.h"
#include "Input.h"
#include "Render.h"
#include "Game.h"

#include "NetSubmit.h" // WiFi + score submit
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// Hardware objects
Adafruit_NeoPixel strip(PIXEL_BUFFER_SIZE, PIN_LED, NEO_GRB + NEO_KHZ800);
Pixel_Grid* pixelGrid = nullptr;
LCD_Panel* lcdPanel = nullptr;

Renderer renderer;
Input input;
TetrisGame game;

enum AppState {
  STATE_TITLE_PIXELCATS,
  STATE_TITLE_TETRIS,
  STATE_PLAYING,
  STATE_GAMEOVER_HOLD
};

AppState state = STATE_TITLE_PIXELCATS;

// Title scroll control (right-to-left)
static const uint16_t TITLE_STEP_MS = 120;
// runtime width of title text in pixels (computed at init)
static int16_t PIXELCATS_TITLE_TEXT_WIDTH = 0;
static int16_t GAMEOVER_TEXT_WIDTH = 0;
static int16_t TETRIS_TITLE_TEXT_WIDTH = 0;
int16_t titleX = W;
uint32_t tTitle = 0;

// Async submission state
static volatile bool submissionInProgress = false;
static bool submissionStarted = false;
static bool submissionSuccess = false;
static String submissionCode = "";
static bool submissionHandled = false;
static TaskHandle_t submitTaskHandle = NULL;
static uint32_t submissionCompleteMs = 0; // millis when submission completed
static const uint32_t SUBMISSION_SHOW_SCORE_MS = 5000; // show PC score for 5s before switching to code (if any)
int16_t lastScore = 0;
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
  state = STATE_TITLE_PIXELCATS;
  titleX = W;
  tTitle = millis();
  input.resetRepeatTimers(millis());
}
static void enterTetris(){
  state = STATE_TITLE_TETRIS;
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
  // initialize scroll position for the Try Again message
  titleX = W;
  tTitle = millis();

  // reset async submission state
  submissionInProgress = false;
  submissionStarted = false;
  submissionSuccess = false;
  submissionCode = "";
  submissionHandled = false;
  submitTaskHandle = NULL;
}

// Background task to submit score without blocking the main loop
static void submitScoreBackgroundTask(void* pvParameters) {
  uint32_t* pscore = (uint32_t*)pvParameters;
  uint32_t score = *pscore;
  delete pscore;

  String code;
  bool ok = submitScoreToServer(score, code);

  submissionSuccess = ok;
  submissionCode = code;
  submissionInProgress = false;

  // finish the task
  vTaskDelete(NULL);
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

  // Compute runtime title width for accurate wrapping (CATS then TETRIS)
  PIXELCATS_TITLE_TEXT_WIDTH = renderer.computeTextPixelWidth("PIXEL CATS");
  TETRIS_TITLE_TEXT_WIDTH = renderer.computeTextPixelWidth("TETRIS");
  GAMEOVER_TEXT_WIDTH = renderer.computeTextPixelWidth("TRY AGAIN");
  submittedThisGame = false;
  enterTitle();
}

void runStandaloneLoop() {
  uint32_t now = millis();

  input.update();
  InputState in = input.sampleEdgesOnly();

  uint8_t maskH = renderer.charToMask('H');
  uint8_t maskE = renderer.charToMask('E');
  uint8_t maskL = renderer.charToMask('L');
  //uint8_t maskL = renderer.charToMask('L');
  uint8_t maskO = renderer.charToMask('O');

  if (state == STATE_TITLE_PIXELCATS){
    if (now - tTitle >= TITLE_STEP_MS) {
      tTitle = now;
      titleX -= 1;
      if (titleX < -PIXELCATS_TITLE_TEXT_WIDTH) titleX = W;
    }
    renderer.drawTitleScroll_PIXELCATS(titleX);

    if (renderer.lcdPanel && renderer.strip) {
      uint32_t pink = renderer.strip->Color(255, 105, 180);
      
      const uint32_t off = renderer.strip->Color(0, 0, 0);

      for (uint8_t d = 0; d < 5; ++d) {
        renderer.lcdPanel->setDigitOnColour(d, pink);
        renderer.lcdPanel->setDigitOffColour(d, off);
      }

      renderer.lcdPanel->setDigitOnColour(5, off);
      renderer.lcdPanel->setDigitOffColour(5, off);


      renderer.lcdPanel->setDigitSegments(0, maskH);
      renderer.lcdPanel->setDigitSegments(1, maskE);
      renderer.lcdPanel->setDigitSegments(2, maskL);
      renderer.lcdPanel->setDigitSegments(3, maskL);
      renderer.lcdPanel->setDigitSegments(4, maskO);
      //renderer.lcdPanel->setDigitSegments(5, cat);
    }
    if (anyStartButtonPressed(in)) {
      enterTetris();
      input.latch();
      return;
    }
  }
  if (state == STATE_TITLE_TETRIS) {
    if (now - tTitle >= TITLE_STEP_MS) {
      tTitle = now;
      titleX -= 1;
      if (titleX < -TETRIS_TITLE_TEXT_WIDTH) titleX = W;
    }

    renderer.drawTitleScroll_TETRIS(titleX);
    if (lastScore > 0){
      if (renderer.lcdPanel && renderer.strip) {
        uint32_t pink = renderer.strip->Color(255, 105, 180);
        uint32_t green = renderer.strip->Color(0, 220, 0);
        const uint32_t off = renderer.strip->Color(0, 0, 0);

        // Set colours per-digit
        renderer.lcdPanel->setDigitOnColour(0, pink);
        renderer.lcdPanel->setDigitOffColour(0, off);
        renderer.lcdPanel->setDigitOnColour(1, pink);
        renderer.lcdPanel->setDigitOffColour(1, off);
        for (uint8_t d = 2; d < 6; ++d) {
          renderer.lcdPanel->setDigitOnColour(d, green);
          renderer.lcdPanel->setDigitOffColour(d, off);
        }

        // Draw 'P' and 'C' using segment masks so letters are guaranteed
        uint8_t maskP = renderer.charToMask('P');
        uint8_t maskC = renderer.charToMask('C');

        renderer.lcdPanel->setDigitSegments(0, maskP);
        renderer.lcdPanel->setDigitSegments(1, maskC);

        // Now set the 4 score digits explicitly using current score as interim
        uint32_t sc = (uint32_t)(lastScore % 10000UL);
        uint8_t d3 = (uint8_t)((sc / 1000) % 10);
        uint8_t d4 = (uint8_t)((sc / 100) % 10);
        uint8_t d5 = (uint8_t)((sc / 10) % 10);
        uint8_t d6 = (uint8_t)((sc / 1) % 10);
        renderer.lcdPanel->setDigitChar(2, (char)('0' + d3));
        renderer.lcdPanel->setDigitChar(3, (char)('0' + d4));
        renderer.lcdPanel->setDigitChar(4, (char)('0' + d5));
        renderer.lcdPanel->setDigitChar(5, (char)('0' + d6));
      }
    }
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
      lastScore = game.score;
      // transition to game-over state; submission and LCD updates are handled there
      enterGameOverHold();
      input.latch();
      return;
    }

    game.render(renderer);
  }
  else if (state == STATE_GAMEOVER_HOLD) {
    if (now - tTitle >= TITLE_STEP_MS) {
      tTitle = now;
      titleX -= 1;
      if (titleX < -GAMEOVER_TEXT_WIDTH) titleX = W;
    }
    renderer.drawGameOverScroll_TRYAGAIN(titleX);

    // Start background submission once when we enter the game-over state
    if (!submissionStarted) {
      submissionStarted = true;
      submissionInProgress = true;
      submissionHandled = false;
      // allocate score on heap to pass into task
      uint32_t* pscore = new uint32_t((uint32_t)game.score);
      // create a background task to perform the HTTP submission
      xTaskCreatePinnedToCore(submitScoreBackgroundTask, "submit", 8192, pscore, 1, &submitTaskHandle, 1);

      // Immediately show PC + score as interim LCD display while submission runs
      if (renderer.lcdPanel && renderer.strip) {
        uint32_t pink = renderer.strip->Color(255, 105, 180);
        uint32_t green = renderer.strip->Color(0, 220, 0);
        const uint32_t off = renderer.strip->Color(0, 0, 0);

        // Set colours per-digit
        renderer.lcdPanel->setDigitOnColour(0, pink);
        renderer.lcdPanel->setDigitOffColour(0, off);
        renderer.lcdPanel->setDigitOnColour(1, pink);
        renderer.lcdPanel->setDigitOffColour(1, off);
        for (uint8_t d = 2; d < 6; ++d) {
          renderer.lcdPanel->setDigitOnColour(d, green);
          renderer.lcdPanel->setDigitOffColour(d, off);
        }

        // Draw 'P' and 'C' using segment masks so letters are guaranteed
        uint8_t maskP = renderer.charToMask('P');
        uint8_t maskC = renderer.charToMask('C');

        renderer.lcdPanel->setDigitSegments(0, maskP);
        renderer.lcdPanel->setDigitSegments(1, maskC);

        // Now set the 4 score digits explicitly using current score as interim
        uint32_t sc = (uint32_t)(game.score % 10000UL);
        uint8_t d3 = (uint8_t)((sc / 1000) % 10);
        uint8_t d4 = (uint8_t)((sc / 100) % 10);
        uint8_t d5 = (uint8_t)((sc / 10) % 10);
        uint8_t d6 = (uint8_t)((sc / 1) % 10);
        renderer.lcdPanel->setDigitChar(2, (char)('0' + d3));
        renderer.lcdPanel->setDigitChar(3, (char)('0' + d4));
        renderer.lcdPanel->setDigitChar(4, (char)('0' + d5));
        renderer.lcdPanel->setDigitChar(5, (char)('0' + d6));
      }
    }
    
    // If submission completed and not yet handled, apply a short delay so player sees PC+score
    if (submissionStarted && !submissionInProgress && !submissionHandled) {
      // record completion time and wait SUBMISSION_SHOW_SCORE_MS before swapping to code
      if (submissionCompleteMs == 0) {
        submissionCompleteMs = millis();
      }

      if ((millis() - submissionCompleteMs) >= SUBMISSION_SHOW_SCORE_MS) {
        submissionHandled = true;
        if (submissionSuccess) {
          // show 6-digit code in white
          if (renderer.lcdPanel && renderer.strip) {
            uint32_t white = renderer.strip->Color(255, 255, 255);
            const uint32_t off = renderer.strip->Color(0, 0, 0);
            for (uint8_t d = 0; d < 6; ++d) {
              renderer.lcdPanel->setDigitOnColour(d, white);
              renderer.lcdPanel->setDigitOffColour(d, off);
            }
          }
          renderer.setDigitsText(submissionCode.c_str()); // show 6-digit code
        } else {
          // submission failed: keep the interim PC+score display (do nothing)
        }
      }
    }

    // Reset submissionCompleteMs once submission restarts (enterGameOverHold resets vars)


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
// void loop()
// {
//   bool gotHostFrame = tryReadHostFrame();

//   if (runtimeMode == MODE_HOST) {
//       input.update();
//       sendHostInputIfChanged(input);
//       input.latch();
//       if (millis() - lastHostFrameMs > HOST_TIMEOUT_MS) {
//         resetHostParser();
//         initStandaloneMode();

//         return;
//       }

//     if (gotHostFrame || hostHasFrame) {
//       renderHostFrame();
//     } else {
//       renderer.setDigitsText("HOST  ");
//       renderer.show();
//     }
//     return;
//   }

//   runStandaloneLoop();
// }
void loop()
{
  bool gotHostFrame = false;
  if (Serial.available() >= 4) { // 'P' + 3
    gotHostFrame = tryReadHostFrame();
  }
  // If we were previously in host mode, enforce timeout even if parsing is currently failing.
  if (runtimeMode == MODE_HOST) {
    if (millis() - lastHostFrameMs > HOST_TIMEOUT_MS) {
      resetHostParser();
      initStandaloneMode();
      return;
    }
  }

  if (runtimeMode == MODE_HOST) {
    input.update();
    sendHostInputIfChanged(input);
    input.latch();

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