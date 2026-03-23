```mermaid

flowchart LR
  subgraph TETRIS[Tetris]
    T_INO[Tetris.ino]
    T_INPUT[Input.h]
    T_GAME[Game.h]
    T_RENDER[Render.h]
    T_HOST[HostRuntime.h]
    T_NET[NetSubmit.h]
    T_PINS[Pins.h]

    T_INO --> T_INPUT --> T_PINS
    T_INO --> T_GAME --> T_PINS
    T_INO --> T_RENDER --> T_PINS
    T_INO --> T_HOST
    T_INO --> T_NET
  end

  subgraph BREAKOUT[Breakout]
    B_INO[Breakout.ino]
    B_INPUT[Input.h and Input.cpp]
    B_GAME[Game.h and Game.cpp]
    B_RENDER[Render.h and Render.cpp]
    B_PINS[Pins.h]

    B_INO --> B_INPUT --> B_PINS
    B_INO --> B_GAME --> B_PINS
    B_INO --> B_RENDER --> B_PINS
  end