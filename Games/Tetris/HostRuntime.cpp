#include "HostRuntime.h"
#include "Render.h" // bring Renderer type into this translation unit

extern Renderer renderer; // renderer is defined in the main sketch (.ino)

volatile RuntimeMode runtimeMode = MODE_STANDALONE;
volatile uint32_t lastHostFrameMs = 0;

const uint32_t HOST_TIMEOUT_MS = 2500;

// 256 LEDs * 3 bytes GRB (matches your C# sender)
uint8_t hostGrb[256 * 3];
volatile bool hostHasFrame = false;
// static bool readExactNonBlocking(uint8_t* dst, size_t n) {
//   if (Serial.available() < (int)n) return false;
//   for (size_t i = 0; i < n; ++i) {
//     int b = Serial.read();
//     if (b < 0) return false;
//     dst[i] = (uint8_t)b;
//   }
//   return true;
// }
void resetHostParser() {
  while (Serial.available() > 0) (void)Serial.read();
  hostHasFrame = false;
  runtimeMode = MODE_STANDALONE;
  lastHostFrameMs = 0;
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

// PBLC handler reads length + payload and updates renderer digits.
// Packet format: 'P' 'B' 'L' 'C' + uint16_t length (little-endian) + payload bytes (UTF-8)
static void handlePblcPacket() {
  uint8_t lenBytes[2];
  if (!readExact(lenBytes, 2, 10)) return;
  uint16_t payloadLen = (uint16_t)lenBytes[0] | ((uint16_t)lenBytes[1] << 8);

  const size_t MAX_LCD = 32;
  uint16_t toRead = payloadLen > MAX_LCD ? MAX_LCD : payloadLen;

  // Read payload into temporary buffer
  char buf[MAX_LCD + 1];
  if (toRead > 0) {
    if (!readExact(reinterpret_cast<uint8_t*>(buf), toRead, 200)) {
      return; // couldn't read payload in time
    }
  }
  buf[toRead] = '\0';

  // If payload was longer than buffer, flush remaining bytes so stream stays in sync
  if (payloadLen > toRead) {
    uint16_t skip = payloadLen - toRead;
    uint8_t tmp;
    while (skip-- > 0) {
      if (!readExact(&tmp, 1, 5)) break;
    }
  }

  // Update LCD via existing API
  renderer.setDigitsText(buf);
  runtimeMode = MODE_HOST;
  lastHostFrameMs = millis();
}
static void handlePb7sPacket() {
  uint8_t lenBytes[2];
  if (!readExact(lenBytes, 2, 10)) return;
  uint16_t payloadLen = (uint16_t)lenBytes[0] | ((uint16_t)lenBytes[1] << 8);

  const uint16_t EXPECT = 15;
  if (payloadLen != EXPECT) {
    // flush payload to keep sync
    uint8_t tmp;
    for (uint16_t i = 0; i < payloadLen; ++i) {
      if (!readExact(&tmp, 1, 5)) break;
    }
    return;
  }

  uint8_t p[EXPECT];
  if (!readExact(p, EXPECT, 50)) return;

  // payload layout:
  // [0..2] masks for digits 0..2
  // [3..11] colors for digits 0..2 as RGB triplets
  // [12..14] score digits (hundreds,tens,ones) as values 0..9
  uint8_t masks3[3] = { p[0], p[1], p[2] };

  uint32_t cols3[3];
  for (uint8_t i = 0; i < 3; ++i) {
    uint8_t r = p[3 + i * 3 + 0];
    uint8_t g = p[3 + i * 3 + 1];
    uint8_t b = p[3 + i * 3 + 2];
    cols3[i] = renderer.strip->Color(r, g, b);
  }

  uint16_t scoreLast3 =
      (uint16_t)(p[12] % 10) * 100 +
      (uint16_t)(p[13] % 10) * 10 +
      (uint16_t)(p[14] % 10);

  renderer.setHostHud3MasksAndScore(masks3, cols3, scoreLast3);
}

bool tryReadHostFrame() {
  const uint32_t tHeader = (runtimeMode == MODE_HOST) ? 10 : 0;
  const uint32_t tLen    = (runtimeMode == MODE_HOST) ? 10 : 0;
  const uint32_t tFrame  = (runtimeMode == MODE_HOST) ? 200 : 0;
  const uint32_t tLcd    = (runtimeMode == MODE_HOST) ? 100 : 0;
  const uint32_t tHud    = (runtimeMode == MODE_HOST) ? 50  : 0;
  const uint16_t expectedLen = sizeof(hostGrb);

  // Loop while there's data to consume; keep iterations fast so loop() isn't stalled.
  while (Serial.available() > 0) {
    // Quick resync: drop until 'P'
    int p = Serial.peek();
    if (p == -1) break;
    if (p != 'P') {
      Serial.read();
      continue;
    }

    // We saw a 'P' at the head. Try to read the next 3 header bytes quickly.
    // Use small timeouts so we don't block for long if stream is partial.
    // We will return false if a required part is not available quickly.
    uint8_t hdr[3];
    if (Serial.available() < 4) return false;

    // consume 'P'
    Serial.read();

    if (!readExact(hdr, 3, tHeader)) {
      // not enough header bytes available yet, give up this attempt and let caller retry
      return false;
    }

    // PBFR (LED frame) handling
    if (hdr[0] == 'B' && hdr[1] == 'F' && hdr[2] == 'R') {
      uint8_t lenBytes[2];
      if (!readExact(lenBytes, 2, tLen)) return false;
      uint16_t len = (uint16_t)lenBytes[0] | ((uint16_t)lenBytes[1] << 8);

      if (len != expectedLen) {
        // bad length; ignore and continue scanning
        continue;
      }

      // Read payload with a modest timeout to avoid long blocking.
      if (!readExact(hostGrb, expectedLen, tFrame)) {
        // payload not available in time; give up attempt now
        return false;
      }
      
      runtimeMode = MODE_HOST;
      lastHostFrameMs = millis();
      hostHasFrame = true;
      return true;
    }
    // PBLC (LCD/text) handling
    else if (hdr[0] == 'B' && hdr[1] == 'L' && hdr[2] == 'C') {
      uint8_t lenBytes[2];
      if (!readExact(lenBytes, 2, tLen)) return false;
      uint16_t payloadLen = (uint16_t)lenBytes[0] | ((uint16_t)lenBytes[1] << 8);

      // Cap to safe buffer size.
      const size_t MAX_LCD = 32;
      uint16_t toRead = payloadLen > MAX_LCD ? MAX_LCD : payloadLen;

      // Read payload with a short bounded timeout so we don't stall the main loop.
      char buf[MAX_LCD + 1];
      if (toRead > 0) {
        if (!readExact(reinterpret_cast<uint8_t*>(buf), toRead, tLcd)) {
          // couldn't read payload in time — give up this pass and retry later
          return false;
        }
      }
      buf[toRead] = '\0';

      // If payloadLen > toRead flush remaining bytes (small timeouts)
      if (payloadLen > toRead) {
        uint16_t skip = payloadLen - toRead;
        uint8_t tmp;
        while (skip-- > 0) {
          if (!readExact(&tmp, 1, 5)) break;
        }
      }

      // Update display — non-blocking, quick
      renderer.setDigitsText(buf);
      runtimeMode = MODE_HOST;
      lastHostFrameMs = millis();
      // keep scanning for more packets/frames in this iteration
      continue;
    }
    else if (hdr[0] == 'B' && hdr[1] == '7' && hdr[2] == 'S') {
      // Only process HUD packets once we're already in host mode.
      // Otherwise a partial PB7S can freeze standalone.
      if (runtimeMode != MODE_HOST) {
        return false;
      }

      handlePb7sPacket();
      runtimeMode = MODE_HOST;
      lastHostFrameMs = millis();
      continue;
    }
    else {
      // unknown packet; continue scanning (we already consumed 'P' + hdr)
      continue;
    }
  }

  return false;
}
