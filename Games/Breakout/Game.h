// Game.h
#pragma once
#include <Arduino.h>
#include "Pins.h"

// per-cell brick colour, 0 = empty
extern uint32_t bricksGrid[BRICK_H][W];

extern int8_t paddleX;
extern bool ballStuck;
extern int8_t ballX, ballY;
extern int8_t ballVX, ballVY;

extern uint32_t score;
extern bool gameOver;

extern uint16_t ballStepMs;
extern uint32_t tBall;
extern uint32_t tBrickDrop;
extern uint16_t bricksHit;

extern uint8_t wheelPos;

void Game_reset();
void Game_movePaddle(int8_t dx);

void Game_serveBall();
void Game_brickDropTick();
void Game_stepBallOnce();

// Small helpers
bool Game_isOver();
