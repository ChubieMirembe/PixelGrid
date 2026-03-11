#pragma once
#include <Arduino.h>

enum RuntimeMode : uint8_t { MODE_STANDALONE = 0, MODE_HOST = 1 };

extern volatile RuntimeMode runtimeMode;
extern volatile uint32_t lastHostFrameMs;
extern const uint32_t HOST_TIMEOUT_MS;

// Host framebuffer (GRB bytes for 256 LEDs)
extern uint8_t hostGrb[256 * 3];
extern volatile bool hostHasFrame;

void resetHostParser();
bool tryReadHostFrame();