# PixelGrid

PixelGrid is an Arduino-based LED matrix game collection built for a custom PixelGrid board. This repo includes playable games, a shared core library, and setup guides to get a first-time user from wiring to uploading sketches.

## What’s in this repo

- **Games/**: Playable sketches (currently **Tetris** and **Breakout**).
- **libraries/**: Local Arduino libraries used by the games, including **PixelGridCore**.
- **pixeltest/**: A hardware verification sketch to confirm the board, LEDs, and buttons are wired correctly.
- **Tutorial/**: Step-by-step setup and upload guides plus a circuit diagram PDF.

## Requirements

- **Arduino IDE** installed on your computer.
- A **PixelGrid board** (or compatible Arduino + LED matrix setup).
- A supported Arduino board such as **Uno**, **Mega 2560**, or **Nano**.
- USB data cable for connecting the board.

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
│   ├── Breakout/
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

## License

No license file is included in this repository. If you plan to distribute or reuse this code, add a license that meets your needs.
