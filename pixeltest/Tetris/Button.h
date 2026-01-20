class Button {
  private: 
  uint16_t mPin;
  uint16_t mPreviousState;
  bool mReleased;
  bool mPressed;
  bool mStateChanged;

  public:
  Button(uint16_t pPin ){
    mPin = pPin;  
    mPreviousState = LOW;
    mReleased = false;
    mPressed = false;
    mStateChanged = false;
  }

  bool isDown() {
    uint16_t buttonState = digitalRead(mPin);
    if(buttonState == HIGH) {
      return true;
    }
    return false;
  }
  bool isUp() {
    uint16_t buttonState = digitalRead(mPin);
    if(buttonState == LOW) {
      return true;
    }
    return false;
  }
  bool released() {    
    return mReleased;
  }
bool pressed() {    
    return mPressed;
  }
  bool stateChanged() {
    return mStateChanged;
  }
  void update() {
    uint16_t buttonState = digitalRead(mPin);
    if(buttonState == HIGH) {
      if(mPreviousState == LOW) {
        // button released
        mReleased = true;
        mStateChanged = true;
      }
      else {
        mReleased = false;
        mStateChanged = false;
      }
    }
    else {
      if(mPreviousState == HIGH) {
        // button pressed
        mPressed = true;
        mStateChanged = true;
      }
      else {
        mPressed = false;
        mStateChanged = false;
      }
    }
    mPreviousState = buttonState;
  }
};

