# PixelGrid

PixelGrid is an Arduino-based LED matrix game collection for a custom PixelGrid board.  
This repository includes playable games, shared libraries, hardware test sketches, and setup/tutorial documentation.

## What’s in this repo

- Games: Playable sketches:
  - [Games/Tetris](Games/Tetris)
  - [Games/Breakout](Games/Breakout)
- Libraries: Local Arduino libraries used by sketches:
  - [libraries/PixelGridcore](libraries/PixelGridcore)
  - [libraries/Adafruit_NeoPixel](libraries/Adafruit_NeoPixel)
  - [libraries/FastLED](libraries/FastLED)
  - [libraries/Firmata](libraries/Firmata)
- Hardware test sketch:
  - [pixeltest/pixeltest.ino](pixeltest/pixeltest.ino)
- Joystick utility sketch:
  - [joysticks/joysticks.ino](joysticks/joysticks.ino)
- Tutorials and setup guides:
  - [Tutorial](Tutorial)
- Host-side tests:
  - [tests](tests)

## Requirements

- Arduino IDE installed.
- PixelGrid board (or compatible board + LED matrix wiring).
- USB data cable.
- Correct board selected in Arduino IDE for the sketch you are uploading.

## How this repo is intended to be used

PixelGrid follows the Arduino Sketchbook layout.  
Set your Arduino Sketchbook location to the repository root so Arduino IDE can discover local libraries in [libraries](libraries) and sketches in [Games](Games), [pixeltest](pixeltest), and [joysticks](joysticks).

## Quick start (first-time setup)

1. Wire the board using:
   - [Tutorial/ESP32-S3 Circuit Diagram.pdf](Tutorial/ESP32-S3%20Circuit%20Diagram.pdf)
2. Set Arduino Sketchbook location to this repository:
   - [Tutorial/setup_sketchbook.md](Tutorial/setup_sketchbook.md)
3. Verify board connection and basic hardware:
   - [Tutorial/setup_board.md](Tutorial/setup_board.md)
4. Upload a game:
   - [Tutorial/setup_upload.md](Tutorial/setup_upload.md)

## Opening sketches in Arduino IDE

Open one of the following:

- Tetris: [Games/Tetris/Tetris.ino](Games/Tetris/Tetris.ino)
- Breakout: [Games/Breakout/Breakout.ino](Games/Breakout/Breakout.ino)
- Pixel hardware test: [pixeltest/pixeltest.ino](pixeltest/pixeltest.ino)
- Joystick readout test: [joysticks/joysticks.ino](joysticks/joysticks.ino)

Then select the correct Board and Port in Tools, and click Upload.

## Local libraries

This repository vendors libraries under [libraries](libraries).  
When Sketchbook location is set to the repository root, these local copies are used by sketches in this project.

## Game and tutorial docs

- Tetris play guide: [Tutorial/tetris_tutorial.md](Tutorial/tetris_tutorial.md)
- Breakout tutorial set:
  - [Tutorial/breakout_tutorial/breakout_beginner.md](Tutorial/breakout_tutorial/breakout_beginner.md)
  - [Tutorial/breakout_tutorial/breakout_intermediate.md](Tutorial/breakout_tutorial/breakout_intermediate.md)
  - [Tutorial/breakout_tutorial/breakout_advanced.md](Tutorial/breakout_tutorial/breakout_advanced.md)

## Tests

Host-based Tetris tests are in [tests](tests).

- Overview: [tests/README.md](tests/README.md)
- Test plan: [tests/TEST_PLAN.md](tests/TEST_PLAN.md)
- Test runner source: [tests/tetris_game_tests.cpp](tests/tetris_game_tests.cpp)

## Project structure

PixelGrid/
- [Games](Games)
  - [Games/Tetris](Games/Tetris)
  - [Games/Breakout](Games/Breakout)
- [libraries](libraries)
  - [libraries/PixelGridcore](libraries/PixelGridcore)
  - [libraries/Adafruit_NeoPixel](libraries/Adafruit_NeoPixel)
  - [libraries/FastLED](libraries/FastLED)
  - [libraries/Firmata](libraries/Firmata)
- [pixeltest](pixeltest)
  - [pixeltest/pixeltest.ino](pixeltest/pixeltest.ino)
- [joysticks](joysticks)
  - [joysticks/joysticks.ino](joysticks/joysticks.ino)
- [tests](tests)
  - [tests/README.md](tests/README.md)
  - [tests/TEST_PLAN.md](tests/TEST_PLAN.md)
  - [tests/tetris_game_tests.cpp](tests/tetris_game_tests.cpp)
- [Tutorial](Tutorial)
  - [Tutorial/ESP32-S3 Circuit Diagram.pdf](Tutorial/ESP32-S3%20Circuit%20Diagram.pdf)
  - [Tutorial/setup_board.md](Tutorial/setup_board.md)
  - [Tutorial/setup_sketchbook.md](Tutorial/setup_sketchbook.md)
  - [Tutorial/setup_upload.md](Tutorial/setup_upload.md)
  - [Tutorial/tetris_tutorial.md](Tutorial/tetris_tutorial.md)
  - [Tutorial/breakout_tutorial](Tutorial/breakout_tutorial)

## Troubleshooting

If upload fails or hardware does not respond, start with:
- [Tutorial/setup_board.md](Tutorial/setup_board.md)
- [Tutorial/setup_upload.md](Tutorial/setup_upload.md)