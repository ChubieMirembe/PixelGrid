#include <Boards.h>
#include <Firmata.h>
#include <FirmataConstants.h>
#include <FirmataDefines.h>
#include <FirmataMarshaller.h>
#include <FirmataParser.h>

#include <Adafruit_NeoPixel.h>
#include "Shape.h"
#include "Pixel_Grid.h"
#include "LCD_Panel.h"
#include "Button.h"

#ifdef __AVR__
#include <avr/power.h>
#endif

#define PIN 6 // Pin where NeoPixels are connected  

#define PIXEL_BUFFER_SIZE 258
#define NUM_ROWS 20
#define NUM_COLUMNS 10

// Example 4 - Receive a number as text and convert it to an int

const byte numChars = 32;
char receivedChars[numChars];   // an array to store the received data

boolean newData = false;

uint32_t dataNumber = 0;             // new for this version

byte receivedBytes[606];
uint16_t byteCount = 0;
uint16_t colourCount = 0;

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_BUFFER_SIZE, PIN, NEO_GRB + NEO_KHZ800);
//Adafruit_NeoPixel strip = Adafruit_NeoPixel(14, PIN, NEO_GRB + NEO_KHZ800);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.
int16_t currentIndex = 0;
uint16_t tailLength = 25;
uint32_t colour;
uint16_t brightness;
int16_t brightnessDirection = 1;
uint16_t brightnessStep = 10;

bool pulse = true;

uint16_t mode = 0;
uint32_t colours[9];
uint32_t randomColours[PIXEL_BUFFER_SIZE];
const int ledPin = 6; // Pin where NeoPixels are connected

uint32_t pixelBuffer[PIXEL_BUFFER_SIZE];
uint16_t pixelGridConversionTable[PIXEL_BUFFER_SIZE];

int count;



unsigned long previousTime;
unsigned long timePassed;
unsigned long timePassed2;
unsigned long timePassed3;
unsigned long timePassed4;



class BagOfSeven {
  private:
    static const uint16_t cSize = 7;
    Shape* mBag[cSize];
    uint16_t mRemainingShapes;

  public:
    BagOfSeven() {
      refreshBag();
    }
    void refreshBag() {
      uint32_t white = strip.Color(255, 255, 255);
      uint32_t light_blue = strip.Color(0, 230, 254);
      uint32_t dark_blue = strip.Color(24, 1, 255);
      uint32_t orange = strip.Color(255, 115, 8);
      uint32_t yellow = strip.Color(255, 222, 0);
      uint32_t green = strip.Color(102, 253, 0);
      uint32_t red = strip.Color(254, 16, 20);
      uint32_t purple = strip.Color(184, 2, 253);
      mBag[0] = Shape::makeI(light_blue);
      mBag[1] = Shape::makeJ(dark_blue);
      mBag[2] = Shape::makeL(orange);
      mBag[3] = Shape::makeO(yellow);
      mBag[4] = Shape::makeS(green);
      mBag[5] = Shape::makeT(purple);
      mBag[6] = Shape::makeZ(red);
      mRemainingShapes = cSize;
    }
    Shape * getNextShape() {
      uint16_t shapeIndex = random(0, mRemainingShapes);
      Shape* shape = mBag[shapeIndex];
      mRemainingShapes--;
      for (uint16_t i = shapeIndex; i < mRemainingShapes; i++) {
        mBag[i] = mBag[i + 1];
      }
      if (mRemainingShapes <= 0) {
        refreshBag();
      }
      return shape;
    }

};





const uint16_t buttonPin1 = 3;     // the number of the pushbutton pin
const uint16_t buttonPin2 = 4;     // the number of the pushbutton pin
const uint16_t buttonPin3 = 5;     // the number of the pushbutton pin

Shape * _currentShape;
LCD_Panel * lcdPanel;
Pixel_Grid * pixelGrid;
Button * leftButton;
Button * rightButton;
Button * fireButton;
uint32_t currentDigit;
Shape* oShape;

void setup() {
  // This is for Trinket 5V 16MHz, you can remove these three lines if you are not using a Trinket
#if defined (__AVR_ATtiny85__)
  if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
#endif


  // End of trinket special code
  pinMode(PIN, OUTPUT);
  pinMode(buttonPin1, INPUT);
  pinMode(buttonPin2, INPUT);
  pinMode(buttonPin3, INPUT);

  leftButton = new Button(buttonPin1);
  rightButton = new Button(buttonPin2);
  fireButton = new Button(buttonPin3);

  count = 0;
  oShape = Shape::makeO(strip.Color(255, 222, 0)); //yellow
  oShape->setPosition(5, 5);

  Serial.begin(115200);
  Serial.flush();
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  initialiseColours();
  lcdPanel = new LCD_Panel(&strip, 214, 6, strip.Color(255, 0, 0));
  pixelGrid = new Pixel_Grid(&strip, 0, 20, 10);
  currentDigit = 0;
  //lcdPanel->changeNumber(currentDigit);
  char score[6];
  score[0] = ' ';
  score[1] = ' ';
  score[2] = ' ';
  score[3] = ' ';
  score[4] = ' ';
  score[0] = ' ';
  lcdPanel->changeCharArray(score);
  //  initialiseRandomColours(pixelGrid);
  //  pixelGrid->render();
  lcdPanel->render();
  pixelGrid->setGridColour(strip.Color(0, 0, 0));
  pixelGrid->render();
  strip.show();
  return;
  colour = getRandomColour();

  //initialisePixelGridConversionTable();


  previousTime = micros();
  timePassed = 0;
  BagOfSeven* bag = new BagOfSeven();
  Serial.println("setup complete");
  for (uint16_t i = 0; i < 15; i++) {
    _currentShape = bag->getNextShape();
    //Serial.println(_currentShape->getName());
    delete _currentShape;
  }



}

void loop() {
  // Some example procedures showing how to display to the pixels:
  unsigned long currentTime = micros();
  unsigned long deltaTime = currentTime - previousTime;

  recvWithEndMarker();
  //   showNewNumber();

  // Serial.println("loop");
  // Serial.println(deltaTime);
  // tailAnimation(deltaTime);
  //
  // return;
  leftButton->update();
  rightButton->update();
  fireButton->update();

  if(leftButton->stateChanged()||rightButton->stateChanged()||fireButton->stateChanged()) {
    sendButtonState();
  }
  //Serial.println(digitalRead(buttonPin1));
  /*
    if(leftButton->released()) {
    pixelGrid->setGridColour(strip.Color(255,0,0));
    }
    if(rightButton->released()) {
    pixelGrid->setGridColour(strip.Color(0,255,0));
    }
    if(fireButton->released()) {
    pixelGrid->setGridColour(strip.Color(0,0,255));
    }
  */
  //pixelGrid->addShape(oShape);
  /*
    // Serial.println(rightButton->isDown());
    if(pulse)
    {
      //pulseAnimation(deltaTime);
     tailAnimation(deltaTime);
    }
    else
    {

      proximityAnimation(deltaTime);
    }
  */
  // countdownAnimation(deltaTime);
  // colour = getRandomColour();
  // pixelGrid->setGridColour(colour);
  // pixelGrid->render();
  //lcdPanel->render();
  // strip.show();

  previousTime = currentTime;
}




void recvWithEndMarker() {
  static byte index = 0;
  char endMarker = '\n';
  char receivedCharacter;
  char scoreMarker = 's';
  char gridMarker = 'g';
  char semiColon = ';';
  char comma = ',';

  if (Serial.available() > 0) {

    if(mode == 0) {
      receivedCharacter = Serial.read();
      if(receivedCharacter == scoreMarker) {
        mode = 1;
      }
    }
    if(mode == 1) {
    if (byteCount < 6) {
      Serial.readBytes(receivedBytes, 6);
      byteCount = 6;
   //   Serial.print("first 6 bytes: ");
    //  Serial.println(byteCount);
      for (uint16_t i = 0; i < 6; i++) {
        uint16_t byteTest = receivedBytes[i];
      //  Serial.print("(");
       // Serial.print(byteTest);
       // Serial.println(")");
      }
    }
    else if (byteCount < 606) {
      Serial.readBytes(receivedBytes + byteCount, 60);
      byteCount += 60;
    //  Serial.print("next 30 bytes: ");
    //  Serial.println(byteCount);
    }
    if (byteCount >= 606) {
      colourCount = 0;
      char score[6];
      for (uint16_t i = 0; i < 6; i++) {
        score[i] = receivedBytes[i];
      }
      String scoreString(score);
     // uint32_t receivedInt = scoreString.toInt();
      //lcdPanel->changeNumber(receivedInt);
      lcdPanel->changeCharArray(score);
      lcdPanel->render();
      for (uint16_t i = 6; i < 606; i += 3) {
        uint16_t index = i;
        uint32_t colourTest = strip.Color(receivedBytes[index], receivedBytes[index + 1], receivedBytes[index + 2]);
        pixelGrid->setGridCellColour(colourCount, colourTest);
        colourCount++;
      }
      pixelGrid->render();
      strip.show();
      byteCount = 0;
      mode = 0;
    }
    }
    // reading the score
    //uint32_t receivedInt = Serial.parseInt();
    /*
      char score[6];
      Serial.readBytes(score, 6);
      Serial.readBytes(receivedBytes, 600);
          String scoreString(score);
      uint32_t receivedInt = scoreString.toInt();
      lcdPanel->changeNumber(receivedInt);
      lcdPanel->render();

      // reading the grid
      for(uint16_t i = 0; i < 600; i+=3) {
      uint32_t colourTest = strip.Color(receivedBytes[i], receivedBytes[i+1], receivedBytes[i+2]);
      pixelGrid->setGridCellColour(colourCount, colourTest);
      colourCount++;
      }
      pixelGrid->render();
      strip.show();

    */



  }
}

byte calculateButtonsState() {
  if(leftButton->isDown()){
    if(rightButton->isDown()) {
      if(fireButton->isDown()){
        return 7; // 111
      }
      else {
        return 6; // 110
      }
    }
    else{
      if(fireButton->isDown()){
        return 5; // 101
      }
      else {
        return 4; // 100
      }
    }
  }
  else {
    if(rightButton->isDown()) {
      if(fireButton->isDown()){
        return 3; // 011
      }
      else {
        return 2; // 010
      }
    }
    else{
      if(fireButton->isDown()){
        return 1; // 001
      }
      else {
        return 0; // 000
      }
    }
  }
}

void sendButtonState() {
  byte buttonState = calculateButtonsState();
  char id = 'b';
 // Serial.print(id);
 // Serial.println(buttonState);
  Serial.write(id);
  Serial.write(buttonState);
}
void countdownAnimation(unsigned long deltaTime)
{
  unsigned long oneSecond = 1000000;
  unsigned long timeBetweenUpdates = oneSecond / 4;
  timePassed = timePassed + deltaTime;

  if (timePassed > timeBetweenUpdates)
  {
    timePassed = 0;

    if (currentDigit < 999999) {
      currentDigit++;
    }
    else {
      currentDigit = 0;
    }
    lcdPanel->changeNumber(currentDigit);
    lcdPanel->render();
    //Serial.println(currentDigit);
  }
}

void initialiseRandomColours(Pixel_Grid * pPixelGrid) {
  for (uint16_t row = 0; row < NUM_ROWS; row++) {
    for (uint16_t column = 0; column < NUM_COLUMNS; column++) {
      pPixelGrid->setGridCellColour(row, column, colours[random(2, 9)]);
    }
  }
}



void initialiseColours() {
  uint32_t black = strip.Color(0, 0, 0);
  uint32_t white = strip.Color(255, 255, 255);
  uint32_t light_blue = strip.Color(0, 230, 254);
  uint32_t dark_blue = strip.Color(24, 1, 255);
  uint32_t orange = strip.Color(255, 115, 8);
  uint32_t yellow = strip.Color(255, 222, 0);
  uint32_t green = strip.Color(102, 253, 0);
  uint32_t red = strip.Color(254, 16, 20);
  uint32_t purple = strip.Color(184, 2, 253);
  colours[0] = black;
  colours[1] = white;
  colours[2] = light_blue;
  colours[3] = dark_blue;
  colours[4] = orange;
  colours[5] = yellow;
  colours[6] = green;
  colours[7] = red;
  colours[8] = purple;
}
void proximityAnimation(unsigned long deltaTime)
{
  unsigned long oneSecond = 1000000;
  unsigned long timeBetweenUpdates = oneSecond;
  timePassed2 = timePassed2 + deltaTime;

  if (timePassed2 > timeBetweenUpdates)
  {
    timePassed2 = 0;
    pulse = true;
    Serial.println("switchProximity");
  }
}

void pulseAnimation(unsigned long deltaTime)
{
  unsigned long oneSecond = 1000000;
  unsigned long timeBetweenUpdates = oneSecond / 100;
  timePassed3 = timePassed3 + deltaTime;
  if (timePassed3 > timeBetweenUpdates)
  {
    brightness = brightness + (brightnessDirection * brightnessStep);
    if (brightness >= 255 || brightness <= 0)
    {
      brightnessDirection = -brightnessDirection;

    }
    if (brightness <= 0)
    {

      pulse = false;
      pixelGrid->clear();
      initialiseRandomColours(pixelGrid);
      Serial.println("switchPulse");

    }
    else {
      for (uint16_t row = 0; row < NUM_ROWS; row++) {
        for (uint16_t column = 0; column < NUM_COLUMNS; column++) {
          uint32_t colour = pixelGrid->getGridCellColour(row, column);
          uint32_t newColour = adjustColourBrightness(colour, brightness);
          pixelGrid->setGridCellColour(row, column, newColour);
        }
      }

    }
    timePassed3 = 0;
  }
}

void tailAnimation(unsigned long deltaTime)
{
  unsigned long oneSecond = 1000000;
  unsigned long timeBetweenUpdates = oneSecond / 1000000;
  timePassed4 = timePassed4 + deltaTime;
  if (timePassed4 > timeBetweenUpdates)
  {
    if (currentIndex == 0)
    {
      colour = colours[random(2, 9)];
      pulse = false;
      pixelGrid->clear();
      initialiseRandomColours(pixelGrid);
      Serial.println("switchTail");
    }
    else {
      pixelGrid->clear();
      showTail(colour, 50);
    }
    currentIndex = currentIndex + 1;
    currentIndex = correctIndex(currentIndex);
    timePassed4 = 0;
  }
}

uint32_t getRandomColour()
{
  uint8_t r = random(0, 255);
  uint8_t g = random(0, 255);
  uint8_t b = random(0, 255);
  uint32_t newColour = strip.Color(r, g, b);
  return newColour;
}


uint32_t adjustColourBrightness(uint8_t r, uint8_t g, uint8_t b, uint16_t brightness)
{
  uint8_t newRed = brightness * r / 255;
  uint8_t newGreen = brightness * g / 255;
  uint8_t newBlue = brightness * b / 255;
  uint32_t newColour = strip.Color(newRed, newGreen, newBlue);
  return newColour;
}

uint32_t adjustColourBrightness(uint32_t colour, uint16_t brightness)
{
  uint8_t r = getPixelColour(colour, 0);
  uint8_t g = getPixelColour(colour, 1);
  uint8_t b = getPixelColour(colour, 2);
  uint32_t newColour = adjustColourBrightness(r, g, b, brightness);
  return newColour;
}

uint8_t getPixelColour(uint32_t mergedColour, uint8_t index)
{
  if (index == 0)
  {
    return getRedComponent(mergedColour);
  }
  else if (index == 1)
  {
    return getGreenComponent(mergedColour);
  }
  else if (index == 2)
  {
    return getBlueComponent(mergedColour);
  }
  return 0;
}

uint8_t getRedComponent(uint32_t mergedColour)
{
  uint8_t r = mergedColour >> 16;
  return r;
}

uint8_t getGreenComponent(uint32_t mergedColour)
{
  uint8_t g = mergedColour >> 8;
  return g;
}

uint8_t getBlueComponent(uint32_t mergedColour)
{
  uint8_t b = mergedColour;
  return b;
}

int16_t correctIndex(int16_t index)
{
  int16_t numPixels = pixelGrid->numPixels();
  if (index >= numPixels)
  {
    index = index - numPixels;
  }
  else if (index < 0)
  {
    index = index + numPixels;
  }
  return index;
}

// Turn all the dots off
void showTail(uint32_t colour, uint8_t wait) {
  uint16_t brightness = 255;

  for (uint16_t i = 0; i < tailLength; i++) {
    int16_t ledIndex = currentIndex - i;
    ledIndex = correctIndex(ledIndex);
    brightness = brightness / 2;
    uint32_t newColour = adjustColourBrightness(colour, brightness);
    pixelGrid->setGridCellColour(ledIndex, newColour);
  }

}



