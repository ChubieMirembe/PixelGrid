 class LCD_Digit {
  private:
  uint32_t mOnColour;
  uint32_t mOffColour;
  uint16_t mCurrentDigit;
  char mCurrentChar;
  uint16_t mStartIndex;
  Adafruit_NeoPixel* mStrip;

  public:LCD_Digit(Adafruit_NeoPixel* pStrip, uint16_t pStartIndex, uint32_t pOnColour){
    mCurrentDigit = 0;
    mCurrentChar = ' ';
    mStrip = pStrip;
    mStartIndex = pStartIndex;
     mOnColour = pOnColour;
    mOffColour = mStrip->Color(0,0,0);
   // mOnColour = mStrip->Color(254,16,20);
   // mOffColour = mStrip->Color(0,0,0);
  }

  void changeNumber(uint16_t pDigit) {
    mCurrentDigit = pDigit;
  }

  void changeChar(char pChar) {
    mCurrentChar = pChar;
  }

  void render() {
    
    switch(mCurrentChar) {
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
      default: generateOff(); break;
    }
  }

  void generateOff() {
    mStrip->setPixelColor(mStartIndex + 0, mOffColour);
    mStrip->setPixelColor(mStartIndex + 1, mOffColour);
    mStrip->setPixelColor(mStartIndex + 2, mOffColour);
    mStrip->setPixelColor(mStartIndex + 3, mOffColour);
    mStrip->setPixelColor(mStartIndex + 4, mOffColour);
    mStrip->setPixelColor(mStartIndex + 5, mOffColour);
    mStrip->setPixelColor(mStartIndex + 6, mOffColour);
  }
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
