#include <cstdio>
#include <cstdint>

#include "Game.h"

namespace {

int failures = 0;

void assertTrue(bool value, const char* expr, const char* file, int line) {
  if (!value) {
    std::printf("FAIL: %s (%s:%d)\n", expr, file, line);
    ++failures;
  }
}

void assertEqU32(uint32_t actual, uint32_t expected, const char* expr, const char* file, int line) {
  if (actual != expected) {
    std::printf("FAIL: %s expected %lu got %lu (%s:%d)\n", expr,
                static_cast<unsigned long>(expected),
                static_cast<unsigned long>(actual),
                file, line);
    ++failures;
  }
}

void assertEqU8(uint8_t actual, uint8_t expected, const char* expr, const char* file, int line) {
  if (actual != expected) {
    std::printf("FAIL: %s expected %u got %u (%s:%d)\n", expr,
                static_cast<unsigned>(expected),
                static_cast<unsigned>(actual),
                file, line);
    ++failures;
  }
}

void assertEqU16(uint16_t actual, uint16_t expected, const char* expr, const char* file, int line) {
  if (actual != expected) {
    std::printf("FAIL: %s expected %u got %u (%s:%d)\n", expr,
                static_cast<unsigned>(expected),
                static_cast<unsigned>(actual),
                file, line);
    ++failures;
  }
}

uint16_t countFilled(const TetrisGame& game) {
  uint16_t count = 0;
  for (uint8_t y = 0; y < PLAY_H; ++y) {
    for (uint8_t x = 0; x < W; ++x) {
      if (game.board[y][x] != 0) {
        ++count;
      }
    }
  }
  return count;
}

}  // namespace

#define ASSERT_TRUE(expr) assertTrue((expr), #expr, __FILE__, __LINE__)
#define ASSERT_EQ_U32(actual, expected) assertEqU32((actual), (expected), #actual, __FILE__, __LINE__)
#define ASSERT_EQ_U16(actual, expected) assertEqU16((actual), (expected), #actual, __FILE__, __LINE__)
#define ASSERT_EQ_U8(actual, expected) assertEqU8((actual), (expected), #actual, __FILE__, __LINE__)

void testValidAtBounds() {
  TetrisGame game{};
  game.clearBoard();
  game.curPiece.type = 0;
  game.curPiece.rot = 0;

  ASSERT_TRUE(game.validAtParams(0, 0, 0, 0));
  ASSERT_TRUE(!game.validAtParams(0, 0, -1, 0));
  ASSERT_TRUE(!game.validAtParams(0, 0, 0, PLAY_H));
}

void testClearLinesSingle() {
  TetrisGame game{};
  game.clearBoard();
  for (uint8_t x = 0; x < W; ++x) {
    game.board[PLAY_H - 1][x] = 1;
  }
  game.board[PLAY_H - 2][0] = 2;

  uint8_t cleared = game.clearLines();

  ASSERT_EQ_U8(cleared, 1);
  ASSERT_EQ_U8(game.board[PLAY_H - 1][0], 2);
}

void testClassicLineClearScore() {
  uint32_t score = TetrisGame::classicLineClearScore(4, 1);
  ASSERT_EQ_U32(score, 2400);
}

void testLevelUpdate() {
  TetrisGame game{};
  game.totalLinesCleared = 9;
  game.level = 0;
  game.fallDelayMs = BASE_FALL_MS;

  game.updateLevelOnCleared(1);

  ASSERT_EQ_U8(game.level, 1);
  uint16_t expected = static_cast<uint16_t>(max(static_cast<uint32_t>(MIN_FALL_MS),
                                                static_cast<uint32_t>(BASE_FALL_MS - FALL_DECREMENT)));
  ASSERT_EQ_U16(game.fallDelayMs, expected);
}

void testHoldLocksAfterUse() {
  TetrisGame game{};
  game.clearBoard();
  game.holdType = -1;
  game.holdLocked = false;
  game.curPiece.type = 1;
  game.curPiece.rot = 0;
  game.nextPiece.type = 2;
  game.nextPiece.rot = 0;

  game.doHold();

  ASSERT_EQ_U8(static_cast<uint8_t>(game.holdType), 1);
  ASSERT_EQ_U8(static_cast<uint8_t>(game.curPiece.type), 2);
  ASSERT_TRUE(game.holdLocked);

  game.curPiece.type = 3;
  game.doHold();

  ASSERT_EQ_U8(static_cast<uint8_t>(game.holdType), 1);
}

void testLockOnFailedMoveDown() {
  TetrisGame game{};
  game.clearBoard();
  game.curPiece.type = 1;
  game.curPiece.rot = 0;
  game.curX = 4;
  game.curY = 0;
  game.nextPiece.type = 0;
  game.nextPiece.rot = 0;

  while (game.validAt(game.curX, game.curY + 1, game.curPiece.rot)) {
    game.curY++;
  }

  bool moved = game.tryMove(0, 1);
  ASSERT_TRUE(!moved);
  ASSERT_TRUE(countFilled(game) > 0);
  ASSERT_TRUE(!game.gameOver);
}

void testSoftDropDelay() {
  TetrisGame game{};
  game.fallDelayMs = 200;
  uint16_t delay = game.currentFallDelay(true);
  ASSERT_EQ_U16(delay, SOFT_DROP_MIN_MS);
}

int main() {
  testValidAtBounds();
  testClearLinesSingle();
  testClassicLineClearScore();
  testLevelUpdate();
  testHoldLocksAfterUse();
  testLockOnFailedMoveDown();
  testSoftDropDelay();

  if (failures == 0) {
    std::printf("All tests passed.\n");
    return 0;
  }

  std::printf("%d test(s) failed.\n", failures);
  return 1;
}
