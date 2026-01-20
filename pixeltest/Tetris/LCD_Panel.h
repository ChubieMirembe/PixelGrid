#include "LCD_Digit.h"
class LCD_Panel {
  private:
  uint32_t mOnColour;
  uint32_t mOffColour;
  uint16_t mNumDigits;
  uint32_t mCurrentNumber;
  uint16_t mStartIndex;
  Adafruit_NeoPixel* mStrip;
  LCD_Digit* mLCDDigits[6];
  char * mCharArray;

  public:
  LCD_Panel(Adafruit_NeoPixel* pStrip, uint16_t pStartIndex, uint16_t pNumDigits, uint32_t pOnColour){
    mCurrentNumber = 0;
    mCharArray = new char[pNumDigits];
    mStrip = pStrip;
    mStartIndex = pStartIndex;
    mNumDigits = pNumDigits;
    mOnColour = pOnColour;
    mOffColour = mStrip->Color(0,0,0);
   // mOnColour = mStrip->Color(254,16,20);
  //  mOffColour = mStrip->Color(0,0,0);
    initialiseDigitArray();
  }

  void initialiseDigitArray() {
    for(uint16_t i = 0; i < mNumDigits; i++) {
      mLCDDigits[i] = new LCD_Digit(mStrip, i*7 + mStartIndex, mOnColour);
    }
  }

  void changeNumber(uint32_t pNumber) {
    mCurrentNumber = pNumber;
    calculateDigits();
  }
  void changeCharArray(char* pCharArray) {
    for (uint16_t i = 0; i < mNumDigits; i++) {
      mCharArray[i] = pCharArray[i];
      sendChars();
    }
  }

  void sendChars() {
    for(uint16_t i = 0; i < mNumDigits; i++) {
      mLCDDigits[i]->changeChar(mCharArray[i]);
    }
  }

  void render() {
    for(uint16_t i = 0; i < mNumDigits; i++) {
      mLCDDigits[i]->render();
    }
  }

  void calculateDigits() {
   // Serial.print("(");
    for(uint16_t i = 1; i <= mNumDigits; i++) {
      float divisor = pow(10, i-1);
      uint16_t digit = (uint32_t)(mCurrentNumber / divisor) % 10;
      mLCDDigits[mNumDigits - i]->changeNumber(digit);
     // Serial.print("[");
     // Serial.print(divisor);
     // Serial.print("]");
     // Serial.print(digit);
    } 
   // Serial.println(")");
  }
};
