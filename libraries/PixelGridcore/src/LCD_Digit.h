#pragma once
#include <Adafruit_NeoPixel.h>
class LCD_Digit {
 private:
  uint32_t mOnColour;
  uint32_t mOffColour;
  uint16_t mCurrentDigit;
  char mCurrentChar;
  uint16_t mStartIndex;
  Adafruit_NeoPixel* mStrip;

  // NEW: raw segment mode
  bool mUseSegmentMask = false;
  uint8_t mSegmentMask = 0; // bits 0..6 correspond to pixels mStartIndex+0..6

  void writeMask(uint8_t mask) {
    for (uint8_t i = 0; i < 7; ++i) {
      uint32_t c = (mask & (1u << i)) ? mOnColour : mOffColour;
      mStrip->setPixelColor(mStartIndex + i, c);
    }
  }

 public:
  LCD_Digit(Adafruit_NeoPixel* pStrip, uint16_t pStartIndex, uint32_t pOnColour) {
    mCurrentDigit = 0;
    mCurrentChar = ' ';
    mStrip = pStrip;
    mStartIndex = pStartIndex;
    mOnColour = pOnColour;
    mOffColour = mStrip->Color(0, 0, 0);
  }

  // NEW: allow per-digit colour control
  void setOnColour(uint32_t c) { mOnColour = c; }
  void setOffColour(uint32_t c) { mOffColour = c; }

  // Existing
  void changeNumber(uint16_t pDigit) {
    mCurrentDigit = pDigit;
  }

  void changeChar(char pChar) {
    mCurrentChar = pChar;
    mUseSegmentMask = false; // NEW: char mode overrides raw mask mode
  }

  // NEW: raw mask mode for custom icons
  void setSegments(uint8_t mask) {
    mSegmentMask = mask;
    mUseSegmentMask = true;
  }

  void clearSegments() {
    mSegmentMask = 0;
    mUseSegmentMask = true;
  }

  void render() {
    if (mUseSegmentMask) {
      writeMask(mSegmentMask);
      return;
    }

    switch (mCurrentChar) {
      case '0': generateLCD_0(); break;
      case '1': generateLCD_1(); break;
      case '2': generateLCD_2(); break;
      case '3': generateLCD_3(); break;
      case '4': generateLCD_4(); break;
      case '5': generateLCD_5(); break;
      case '6': generateLCD_6(); break;
      case '7': generateLCD_7(); break;
      case '8': generateLCD_8(); break;
      case '9': generateLCD_9(); break;
      default:  generateOff();   break;
    }
  }

  void generateOff() { writeMask(0); }

  // Keep your existing generateLCD_* functions exactly as-is below
  void generateLCD_0() {
    mStrip->setPixelColor(mStartIndex + 0, mOnColour);
    mStrip->setPixelColor(mStartIndex + 1, mOnColour);
    mStrip->setPixelColor(mStartIndex + 2, mOnColour);
    mStrip->setPixelColor(mStartIndex + 3, mOnColour);
    mStrip->setPixelColor(mStartIndex + 4, mOnColour);
    mStrip->setPixelColor(mStartIndex + 5, mOnColour);
    mStrip->setPixelColor(mStartIndex + 6, mOffColour);
  }
  void generateLCD_1() {
    mStrip->setPixelColor(mStartIndex + 0, mOffColour);
    mStrip->setPixelColor(mStartIndex + 1, mOffColour);
    mStrip->setPixelColor(mStartIndex + 2, mOnColour);
    mStrip->setPixelColor(mStartIndex + 3, mOnColour);
    mStrip->setPixelColor(mStartIndex + 4, mOffColour);
    mStrip->setPixelColor(mStartIndex + 5, mOffColour);
    mStrip->setPixelColor(mStartIndex + 6, mOffColour);
  }
  void generateLCD_2() {
    mStrip->setPixelColor(mStartIndex + 0, mOffColour);
    mStrip->setPixelColor(mStartIndex + 1, mOnColour);
    mStrip->setPixelColor(mStartIndex + 2, mOnColour);
    mStrip->setPixelColor(mStartIndex + 3, mOffColour);
    mStrip->setPixelColor(mStartIndex + 4, mOnColour);
    mStrip->setPixelColor(mStartIndex + 5, mOnColour);
    mStrip->setPixelColor(mStartIndex + 6, mOnColour);
  }
  void generateLCD_3() {
    mStrip->setPixelColor(mStartIndex + 0, mOffColour);
    mStrip->setPixelColor(mStartIndex + 1, mOnColour);
    mStrip->setPixelColor(mStartIndex + 2, mOnColour);
    mStrip->setPixelColor(mStartIndex + 3, mOnColour);
    mStrip->setPixelColor(mStartIndex + 4, mOnColour);
    mStrip->setPixelColor(mStartIndex + 5, mOffColour);
    mStrip->setPixelColor(mStartIndex + 6, mOnColour);
  }
  void generateLCD_4() {
    mStrip->setPixelColor(mStartIndex + 0, mOnColour);
    mStrip->setPixelColor(mStartIndex + 1, mOffColour);
    mStrip->setPixelColor(mStartIndex + 2, mOnColour);
    mStrip->setPixelColor(mStartIndex + 3, mOnColour);
    mStrip->setPixelColor(mStartIndex + 4, mOffColour);
    mStrip->setPixelColor(mStartIndex + 5, mOffColour);
    mStrip->setPixelColor(mStartIndex + 6, mOnColour);
  }
  void generateLCD_5() {
    mStrip->setPixelColor(mStartIndex + 0, mOnColour);
    mStrip->setPixelColor(mStartIndex + 1, mOnColour);
    mStrip->setPixelColor(mStartIndex + 2, mOffColour);
    mStrip->setPixelColor(mStartIndex + 3, mOnColour);
    mStrip->setPixelColor(mStartIndex + 4, mOnColour);
    mStrip->setPixelColor(mStartIndex + 5, mOffColour);
    mStrip->setPixelColor(mStartIndex + 6, mOnColour);
  }
  void generateLCD_6() {
    mStrip->setPixelColor(mStartIndex + 0, mOnColour);
    mStrip->setPixelColor(mStartIndex + 1, mOnColour);
    mStrip->setPixelColor(mStartIndex + 2, mOffColour);
    mStrip->setPixelColor(mStartIndex + 3, mOnColour);
    mStrip->setPixelColor(mStartIndex + 4, mOnColour);
    mStrip->setPixelColor(mStartIndex + 5, mOnColour);
    mStrip->setPixelColor(mStartIndex + 6, mOnColour);
  }
  void generateLCD_7() {
    mStrip->setPixelColor(mStartIndex + 0, mOffColour);
    mStrip->setPixelColor(mStartIndex + 1, mOnColour);
    mStrip->setPixelColor(mStartIndex + 2, mOnColour);
    mStrip->setPixelColor(mStartIndex + 3, mOnColour);
    mStrip->setPixelColor(mStartIndex + 4, mOffColour);
    mStrip->setPixelColor(mStartIndex + 5, mOffColour);
    mStrip->setPixelColor(mStartIndex + 6, mOffColour);
  }
  void generateLCD_8() {
    mStrip->setPixelColor(mStartIndex + 0, mOnColour);
    mStrip->setPixelColor(mStartIndex + 1, mOnColour);
    mStrip->setPixelColor(mStartIndex + 2, mOnColour);
    mStrip->setPixelColor(mStartIndex + 3, mOnColour);
    mStrip->setPixelColor(mStartIndex + 4, mOnColour);
    mStrip->setPixelColor(mStartIndex + 5, mOnColour);
    mStrip->setPixelColor(mStartIndex + 6, mOnColour);
  }
  void generateLCD_9() {
    mStrip->setPixelColor(mStartIndex + 0, mOnColour);
    mStrip->setPixelColor(mStartIndex + 1, mOnColour);
    mStrip->setPixelColor(mStartIndex + 2, mOnColour);
    mStrip->setPixelColor(mStartIndex + 3, mOnColour);
    mStrip->setPixelColor(mStartIndex + 4, mOnColour);
    mStrip->setPixelColor(mStartIndex + 5, mOffColour);
    mStrip->setPixelColor(mStartIndex + 6, mOnColour);
  }
};