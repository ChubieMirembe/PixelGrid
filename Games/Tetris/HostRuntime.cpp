#include "HostRuntime.h"

volatile RuntimeMode runtimeMode = MODE_STANDALONE;
volatile uint32_t lastHostFrameMs = 0;

const uint32_t HOST_TIMEOUT_MS = 2500;

// 256 LEDs * 3 bytes GRB (matches your C# sender)
uint8_t hostGrb[256 * 3];
volatile bool hostHasFrame = false;

void resetHostParser() {
  while (Serial.available() > 0) (void)Serial.read();
  hostHasFrame = false;
}

static bool readExact(uint8_t* dst, size_t n, uint32_t timeoutMs) {
  uint32_t start = millis();
  size_t got = 0;
  while (got < n) {
    if (Serial.available() > 0) {
      int b = Serial.read();
      if (b >= 0) dst[got++] = (uint8_t)b;
    } else {
      if (millis() - start > timeoutMs) return false;
      delay(1);
    }
  }
  return true;
}

bool tryReadHostFrame() {
  const uint16_t expectedLen = sizeof(hostGrb);

  while (Serial.available() > 0) {
    int b = Serial.read();
    if (b != 'P') continue;

    uint8_t hdr[3];
    if (!readExact(hdr, 3, 5)) return false;

    // Expect "BFR"
    if (hdr[0] != 'B' || hdr[1] != 'F' || hdr[2] != 'R') continue;

    uint8_t lenBytes[2];
    if (!readExact(lenBytes, 2, 10)) return false;

    uint16_t len = (uint16_t)lenBytes[0] | ((uint16_t)lenBytes[1] << 8);
    if (len != expectedLen) {
      // bad length; ignore and resync
      continue;
    }

    if (!readExact(hostGrb, expectedLen, 50)) return false;

    runtimeMode = MODE_HOST;
    lastHostFrameMs = millis();
    hostHasFrame = true;
    return true;
  }

  return false;
}