#include <Adafruit_NeoPixel.h>

#define PIN 5                 // Change if needed
#define PIXEL_BUFFER_SIZE 265

Adafruit_NeoPixel strip(PIXEL_BUFFER_SIZE, PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  strip.begin();
  strip.setBrightness(30);     // Keep low while testing power
  strip.clear();

  for (uint16_t i = 0; i < PIXEL_BUFFER_SIZE; i++) {
    strip.setPixelColor(i, strip.Color(0, i, i));  // Green
  }

  strip.show();
}

void loop() {
  // Nothing here — LEDs stay green
}