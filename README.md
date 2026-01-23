# PixelGrid

PixelGrid is an Arduino-based LED matrix game collection built for a custom PixelGrid board. This repo includes playable games, a shared core library, and setup guides to get a first-time user from wiring to uploading sketches.

## What’s in this repo

- **Games/**: Playable sketches (currently **Tetris**).
- **libraries/**: Local Arduino libraries used by the games, including **PixelGridCore**.
- **pixeltest/**: A hardware verification sketch to confirm the board, LEDs, and buttons are wired correctly.
- **Tutorial/**: Step-by-step setup, upload guides, and the tutorial for building your own game.

## Requirements

- **Arduino IDE** installed on your computer.
- A **PixelGrid board** (or compatible Arduino + LED matrix setup).
- A supported Arduino board such as **Uno**, **Mega 2560**, or **Nano**.
- USB data cable for connecting the board.

## How to work with this repo

PixelGrid uses Arduino's Sketchbook layout, so you will point the Arduino Sketchbook location to the repo root. That lets the IDE pick up the local libraries in `libraries/` and the sketches in `Games/` and `pixeltest/`.

If you're building your own game, start in the tutorial where you create a new sketch and use the PixelGridCore library. The tutorial is the canonical place for writing your first game.

### Opening sketches in Arduino IDE

1. Launch **Arduino IDE**.
2. Open a sketch from this repo:
   - Game: `Games/Tetris/Tetris.ino`
   - Hardware test: `pixeltest/pixeltest.ino`
3. Select the correct **Board** and **Port** in **Tools**.
4. Click **Upload**.

### Local libraries

This repo vendors its Arduino libraries under `libraries/`. When the Sketchbook location is set to the repo root, these libraries are available without any additional installs.
If you already have system-wide copies of the same libraries, the local versions here will be used for the sketches in this repo.

## Quick start (first-time setup)

1. **Wire the board** using the circuit diagram:
   - `Tutorial/Circuit Diagram.pdf`
2. **Set the Arduino Sketchbook location** to this repo so the local libraries are detected:
   - Follow `Tutorial/setup_sketchbook.md`.
3. **Connect the board** and confirm Arduino recognizes it:
   - Select the correct board and port in Arduino IDE.
4. **Run the pixel test sketch** to verify LEDs and buttons:
   - Follow `Tutorial/setup_board.md`.
5. **Upload a game** (example: Tetris):
   - Follow `Tutorial/setup_upload.md`.

## Creating your own game

1. Open the tutorial and follow the guide for creating a new sketch:
   - `Tutorial/setup_upload.md`
2. Use the `PixelGridCore` library from `libraries/PixelGridcore` for the display, input, and shared utilities.
3. Place your new game in `Games/YourGameName/YourGameName.ino`.

## Uploading a game

1. Open Arduino IDE.
2. Open the game sketch, for example:
   - `Games/Tetris/Tetris.ino`
3. Select the correct **Board** and **Port** in **Tools**.
4. Click **Upload**.

See the full walkthrough in `Tutorial/setup_upload.md`.

## Project structure

```
PixelGrid/
├── Games/
│   └── Tetris/
├── libraries/
│   ├── Adafruit_NeoPixel/
│   ├── FastLED/
│   └── PixelGridcore/
├── pixeltest/
│   └── pixeltest.ino
└── Tutorial/
    ├── Circuit Diagram.pdf
    ├── setup_board.md
    ├── setup_sketchbook.md
    └── setup_upload.md
```

## Troubleshooting

If the board does not light up or upload fails, see the troubleshooting steps in:
- `Tutorial/setup_board.md`
