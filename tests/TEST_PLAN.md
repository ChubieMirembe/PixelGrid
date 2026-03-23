# Tetris Host Unit Tests — Test Plan

## Scope
- Validate the host-based unit tests in `tests/tetris_game_tests.cpp` that exercise
  core Tetris gameplay logic without Arduino hardware dependencies.

## Objectives
- Ensure core gameplay rules (collision checks, line clears, scoring, leveling,
  hold mechanics, and drop timing) behave as expected on a host machine.
- Provide a fast regression signal locally and in CI for changes to Tetris logic.

## Test Strategy
- Build and run a C++ test runner on a host machine with stubbed Arduino and
  PixelGrid headers.
- Execute deterministic tests that cover critical game logic paths.

## Test Environment
- Local: Any system with `g++` and C++17 support.
- CI: GitHub Actions workflow `.github/workflows/tetris-tests.yml` on
  `ubuntu-latest`.

## Test Cases
| ID | Area | Description | Coverage (test name) |
| --- | --- | --- | --- |
| TET-001 | Collision bounds | Validate placement bounds for left, bottom edges. | `testValidAtBounds` |
| TET-002 | Line clears | Clear a single line and verify row shift. | `testClearLinesSingle` |
| TET-003 | Scoring | Verify classic line-clear scoring. | `testClassicLineClearScore` |
| TET-004 | Leveling | Verify level increments and fall delay updates. | `testLevelUpdate` |
| TET-005 | Hold | Verify hold locks after use and preserves held type. | `testHoldLocksAfterUse` |
| TET-006 | Lock on drop | Verify lock and board fill on failed downward move. | `testLockOnFailedMoveDown` |
| TET-007 | Soft drop timing | Verify soft drop uses minimum delay. | `testSoftDropDelay` |

## Entry / Exit Criteria
- **Entry:** Source changes touching `Games/Tetris/**` or `tests/**` are ready.
- **Exit:** All tests pass locally and in CI.

## Execution
```sh
g++ -std=c++17 -I tests/stubs -I Games/Tetris tests/tetris_game_tests.cpp -o tests/tetris_game_tests
./tests/tetris_game_tests
```

## Reporting
- Local: Test runner prints failures to stdout.
- CI: GitHub Actions job fails on non-zero exit.
