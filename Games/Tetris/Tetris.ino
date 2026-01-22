// Tetris.ino
#include <Adafruit_NeoPixel.h>
#include <avr/pgmspace.h>
#include <PixelGridCore.h>

#include "Pins.h"
#include "Input.h"
#include "Game.h"
#include "Render.h"

// Hardware objects (Render.h uses extern)
Adafruit_NeoPixel strip(PIXEL_BUFFER_SIZE, PIN_LED, NEO_GRB + NEO_KHZ800);
Pixel_Grid* pixelGrid = nullptr;
LCD_Panel* lcdPanel = nullptr;

// Score digits helper (Game.h declares extern)
void updateScoreDigits(uint32_t s) {
  char tmp[7];
  char out[6];
  snprintf(tmp, sizeof(tmp), "%6lu", (unsigned long)s);
  for (uint8_t i = 0; i < 6; ++i) out[i] = tmp[i];
  lcdPanel->changeCharArray(out);
}

void setup() {
  randomSeed(analogRead(A0));
  Serial.begin(115200);

  strip.begin();
  strip.show();

  pixelGrid = new Pixel_Grid(&strip, 0, MATRIX_ROWS, W);
  lcdPanel  = new LCD_Panel(&strip, 214, 6, strip.Color(255, 255, 255));

  renderInitColours();

  inputInit();

  updateScoreDigits(0);
  lcdPanel->render();

  drawBackgroundRows();
  pixelGrid->render();
  strip.show();

  gameInit();
}

void loop() {
  // Read controls
  inputUpdate();

  // Reset if game over and any rotate button pressed
  if (gameOver) {
    if (anyRotatePressed()) gameReset();

    renderFrame();
    inputLatch();
    delay(10);
    return;
  }

  // Update game rules/state
  gameUpdate();

  // Draw
  renderFrame();

  // Latch edges
  inputLatch();

  delay(10);
}
