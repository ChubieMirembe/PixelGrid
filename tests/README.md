# Tetris Unit Tests (Host Build)

These tests compile the `TetrisGame` logic on a host machine using stubbed
Arduino/renderer headers.

## Build & Run

```sh
g++ -std=c++17 -I tests/stubs -I Games/Tetris tests/tetris_game_tests.cpp -o tests/tetris_game_tests
./tests/tetris_game_tests
```

## CI (on push)

These tests run automatically on push and pull request via
`.github/workflows/tetris-tests.yml`.

## Test Plan

See `tests/TEST_PLAN.md` for scope, cases, and execution details.
