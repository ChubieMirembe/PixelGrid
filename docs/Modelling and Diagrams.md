# 06. Modelling and Diagrams

## 1. System context diagram

```mermaid
flowchart LR
    Player[Player] -->|button and joystick input| PixelGrid[PixelGrid embedded game device]
    PixelGrid -->|LED output| Matrix[10 x 20 LED matrix]
    PixelGrid -->|score, text, code output| Segment[Six-digit segment panel]
    PixelGrid <-->|optional serial frames and input byte| Host[Host PC application]
    PixelGrid -->|optional HTTPS JSON POST| ScoreService[External score-code service]
    Developer[Developer] -->|Arduino IDE upload| PixelGrid
    Developer -->|g++ tests and GitHub Actions| Repo[PixelGrid repository]
```

## 2. Container/component diagram

```mermaid
flowchart TB
    subgraph Repo[PixelGrid repository]
      subgraph Games[Games]
        Tetris[Tetris sketch and modules]
        Breakout[Breakout sketch and modules]
      end
      Core[PixelGridCore local Arduino library]
      Vendor[Vendored Arduino libraries]
      Tests[Host tests]
      CI[GitHub Actions workflow]
      Docs[Tutorial and evidence documentation]
    end

    Tetris --> Core
    Breakout --> Core
    Core --> Vendor
    Tests --> Tetris
    CI --> Tests
    Docs --> Games

    subgraph TetrisComponents[Tetris components]
      TIno[Tetris.ino]
      TInput[Input.h]
      TGame[Game.h]
      TRender[Render.h]
      THost[HostRuntime.cpp]
      TNet[NetSubmit.h]
      TPins[Pins.h]
    end

    TIno --> TInput
    TIno --> TGame
    TIno --> TRender
    TIno --> THost
    TIno --> TNet
    TInput --> TPins
    TGame --> TPins
    TRender --> Core
```

## 3. Tetris gameplay sequence diagram

```mermaid
sequenceDiagram
    actor Player
    participant Main as "Tetris Main"
    participant Input as "Input.h"
    participant Game as "TetrisGame"
    participant Renderer as "Renderer"
    participant Grid as "Pixel_Grid"
    participant Panel as "LCD_Panel"

    Player->>Input: Press rotate/move/drop/hold controls

    Main->>Input: update()
    Main->>Input: state()
    Input-->>Main: Debounced InputState

    Main->>Input: horizontalRepeat(now)
    Input-->>Main: repeatDx

    Main->>Game: update(inputState, repeatDx, now, renderer)

    alt Hold pressed
        Game->>Game: doHold()
    end

    alt Rotation pressed
        Game->>Game: tryRotateTo()
    end

    alt Movement or gravity
        Game->>Game: validAt() and tryMove()
    end

    alt Piece locks
        Game->>Game: lockPiece(), clearLines(), score, spawnNext()
    end

    Game->>Renderer: setHudHoldNextScore()

    Main->>Game: render(renderer)

    Game->>Renderer: draw board, ghost, current piece

    Renderer->>Grid: setGridCellColour()
    Renderer->>Panel: setDigit segments/chars/colours

    Renderer->>Grid: render/show
```

## 4. Tetris activity diagram

```mermaid
flowchart TD
    Start([Loop tick]) --> ReadInput[Read and debounce input]
    ReadInput --> HostCheck{Host mode active?}
    HostCheck -- Yes --> ReadSerial[Try read host frame]
    ReadSerial --> HostTimeout{Host timeout?}
    HostTimeout -- Yes --> Standalone[Return to standalone mode]
    HostTimeout -- No --> RenderHost[Render host frame or HOST text]
    RenderHost --> End([End tick])
    HostCheck -- No --> StateCheck{Standalone state}
    StateCheck -- Title --> RenderTitle[Render title animation]
    RenderTitle --> StartPressed{Start pressed?}
    StartPressed -- Yes --> ResetGame[Reset Tetris game]
    StartPressed -- No --> Latch[Latch input]
    ResetGame --> Playing[Playing state]
    StateCheck -- Playing --> UpdateGame[Update movement, gravity, hold, rotate, scoring]
    UpdateGame --> GameOver{Game over?}
    GameOver -- No --> RenderGame[Render playfield and HUD]
    GameOver -- Yes --> EnterGameOver[Enter game-over hold]
    StateCheck -- GameOver --> Submit{Submission started?}
    Submit -- No --> StartTask[Start background score submission]
    Submit -- Yes --> ShowStatus[Show interim score or returned code]
    EnterGameOver --> ShowStatus
    StartTask --> ShowStatus
    RenderGame --> Latch
    ShowStatus --> Restart{Restart input?}
    Restart -- Yes --> RenderTitle
    Restart -- No --> Latch
    Latch --> End
```

## 5. Score submission sequence diagram

```mermaid
sequenceDiagram
    participant GameOver as Tetris game-over logic
    participant Task as submitScoreBackgroundTask
    participant WiFi as Wi-Fi and EAP setup
    participant Time as NTP time
    participant Crypto as mbedTLS HMAC
    participant HTTP as HTTPClient
    participant API as External /api/codes
    participant Display as LCD panel

    GameOver->>Task: Create task with score
    Task->>WiFi: netEnsureWifi()
    WiFi->>Time: netEnsureTime()
    Time-->>WiFi: Valid Unix time
    Task->>Crypto: computeDeviceSig(secret, game_code, score, ts, nonce)
    Crypto-->>Task: base64url signature
    Task->>HTTP: POST JSON to API_BASE + /api/codes
    HTTP->>API: score, game_code, ts, nonce, sig
    API-->>HTTP: 2xx JSON containing code
    HTTP-->>Task: response body
    Task->>Task: extractJsonStringField(response, code)
    Task->>Display: Store code for delayed display
```

## 6. Serial host packet flow

```mermaid
sequenceDiagram
    participant Host as Host PC
    participant Serial as Serial stream
    participant Parser as HostRuntime parser
    participant Renderer as Renderer
    participant Input as Device input

    Host->>Serial: PBFR + length + GRB frame bytes
    Parser->>Serial: Read and validate header/length
    Parser->>Renderer: Update host frame buffer
    Host->>Serial: PBLC + length + text bytes
    Parser->>Renderer: setDigitsText(text)
    Host->>Serial: PB7S + length + 15-byte HUD payload
    Parser->>Renderer: setHostHud3MasksAndScore(...)
    Input->>Serial: b + packed input byte when state changes
    Serial-->>Host: Device input notification
```

## 7. Runtime data model diagram

```mermaid
erDiagram
    GAME_SKETCH ||--|| INPUT_MODULE : reads
    GAME_SKETCH ||--|| GAME_LOGIC : updates
    GAME_SKETCH ||--|| RENDERER : calls
    GAME_LOGIC ||--|| GAME_STATE : mutates
    RENDERER ||--|| PIXEL_GRID : writes
    RENDERER ||--|| LCD_PANEL : writes
    PIXEL_GRID ||--|| NEOPIXEL_STRIP : renders
    LCD_PANEL ||--|{ LCD_DIGIT : owns

    GAME_STATE {
      array board_or_bricks
      int current_positions
      uint32 score
      bool gameOver
      timing_values timers
    }

    INPUT_MODULE {
      bool stable
      bool prevStable
      uint32 lastChange
      repeat_timers movement
    }

    RENDERER {
      colours palette
      mapping_helpers coordinates
      text_helpers display
    }
```

## 8. Evidence gap diagram

```mermaid
flowchart LR
    Firmware[PixelGrid firmware] -->|POST /api/codes| MissingBackend[Backend not present]
    MissingBackend --> MissingDB[Database/schema not present]
    Firmware -->|includes| MissingConfig[NetConfig.h not present]
    Repo[Repository] --> MissingManifest[No board/dependency lock manifest]
    Repo --> MissingOpenAPI[No OpenAPI/Swagger document]
```
