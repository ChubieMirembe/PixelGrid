// Render.h
#pragma once
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <PixelGridCore.h>
#include "Pins.h"

// Hardware objects (global)
extern Adafruit_NeoPixel strip;
extern Pixel_Grid* pixelGrid;
extern LCD_Panel* lcdPanel;

// Colours (global)
extern uint32_t PLAY_BG_COLOR_U32;
extern uint32_t PADDLE_COLOR_U32;
extern uint32_t BALL_COLOR_U32;

void Render_begin();
void Render_updateScoreDigits(uint32_t s);
uint32_t Render_wheelColor(uint8_t pos);

// Renders the whole frame from current game state (implemented in Render.cpp)
void Render_renderFrame();
