### Tetris Class Diagram

```mermaid
classDiagram
direction LR

class Btn {
  +pin: uint8_t
  +stable: bool
  +prevStable: bool
  +lastRaw: bool
  +lastChange: uint32_t
  +begin(p)
  +update()
  +pressedEdge()
  +releasedEdge()
  +latch()
}

class InputState {
  +rotLeftPressed: bool
  +rotRightPressed: bool
  +holdPressed: bool
  +anyButtonPressed: bool
  +leftHeld: bool
  +rightHeld: bool
  +downHeld: bool
}

class Input {
  +btn1: Btn
  +btn2: Btn
  +btn3: Btn
  +btn4: Btn
  +joyU: Btn
  +joyL: Btn
  +joyR: Btn
  +joyD: Btn
  +begin()
  +update()
  +sampleEdgesOnly() InputState
  +joystickRepeatDx(now) int8_t
  +latch()
  +resetRepeatTimers(now)
}

class TetrisGame {
  +board[PLAY_H][W]
  +curPiece
  +nextPiece
  +holdType
  +score
  +level
  +fallDelayMs
  +initColours(renderer)
  +reset(renderer)
  +tick(inputState, now, renderer)
  +render(renderer)
}

class Renderer {
  +begin(strip, pixelGrid, lcdPanel)
  +renderGame(game)
  +setScoreDigits(score)
  +setDigitsText(text)
  +computeTextPixelWidth(text) int16_t
}

class HostRuntime {
  +initStandaloneMode()
  +runStandaloneLoop()
  +runHostLoop()
}

class NetSubmit {
  +submitScoreToServer(score, code) bool
}

Input o-- Btn : contains
Input --> InputState : produces
HostRuntime --> Input : uses
HostRuntime --> TetrisGame : drives
HostRuntime --> Renderer : draws
HostRuntime --> NetSubmit : submits score
TetrisGame --> Renderer : score and board output