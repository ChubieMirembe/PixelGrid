

```mermaid

classDiagram
direction LR

class Pins {
  <<module>>
  +PIN_LED
  +PIN_BTN1
  +PIN_BTN2
  +PIN_BTN3
  +PIN_BTN4
  +PIN_JOY_UP
  +PIN_JOY_LEFT
  +PIN_JOY_RIGHT
  +PIN_JOY_DOWN
  +FRAME_MS
  +BRICK_DROP_MS
}

class BreakoutMain {
  <<module>>
  +setup()
  +loop()
}

class InputModule {
  <<module>>
  +Input_begin()
  +Input_update()
  +Input_latch()
  +Input_servePressedEdge() bool
  +Input_paddleStepFromJoystickRepeat(now) int8_t
}

class GameModule {
  <<module>>
  +Game_reset()
  +Game_movePaddle(dx)
  +Game_serveBall()
  +Game_brickDropTick()
  +Game_stepBallOnce()
  +Game_isOver() bool
}

class RenderModule {
  <<module>>
  +Render_begin()
  +Render_updateScoreDigits(score)
  +Render_wheelColor(pos) uint32_t
  +Render_renderFrame()
}

BreakoutMain --> InputModule : reads controls
BreakoutMain --> GameModule : updates state
BreakoutMain --> RenderModule : renders frame
InputModule --> Pins : uses pin map
GameModule --> Pins : uses constants
RenderModule --> Pins : uses hardware constants
GameModule --> RenderModule : score updates