#define PIXEL_BUFFER_SIZE 258
#include "Shape.h"
class Shape;
class Pixel_Grid {
  private:
  uint32_t mOnColour;
  uint32_t mOffColour;
  uint16_t mNumRows;
  uint16_t mNumColumns;
  uint16_t mPixelGridSize;
  uint32_t mCurrentNumber;
  uint16_t mStartIndex;
  Adafruit_NeoPixel* mStrip;
  uint32_t mPixelBuffer[PIXEL_BUFFER_SIZE];
  uint16_t mPixelGridConversionTable[PIXEL_BUFFER_SIZE];    
  

  public:
  Pixel_Grid(Adafruit_NeoPixel* pStrip, uint16_t pStartIndex, uint16_t pNumRows, uint16_t pNumColumns){
    mStrip = pStrip;
    mStartIndex = pStartIndex;
    mNumRows = pNumRows;
    mNumColumns = pNumColumns;
    this->initialisePixelGridConversionTable();
    mPixelGridSize = mNumRows * mNumColumns;
  }

  void addShape(Shape* pShape) {
    uint16_t left = pShape->getX();
    uint16_t bottom = pShape->getY();
    uint16_t numRows = pShape->numRows();
    uint16_t numColumns = pShape->numColumns();
    
    for(uint16_t x = 0; x < numColumns; x++) {
      uint16_t offsetX = x + left;
      for(uint16_t y = 0; y < numRows; y++) {
        uint16_t offsetY = y + bottom;
        setGridCellColour(offsetY, offsetX, pShape->getColour(y, x));
        
      } 
    }


  }

  void initialisePixelGridConversionTable() {
  int16_t columnDirection = 1;
  for(uint16_t column = 0; column < mNumColumns; column++) {
    int16_t indexModifier = mNumRows-1;
    int16_t columnStart = 0;
    if(columnDirection < 0) {
      columnStart = mNumRows -1;
    }
    for(uint16_t row = 0; row < mNumRows; row++) {
        
      uint16_t index = this->getIndexFromRowAndColumn(row, column);
     // int16_t ledIndex = ((mNumColumns - 1) - column) * mNumRows + columnStart;
     int16_t ledIndex = (column) * mNumRows + columnStart;
      int16_t ledIndex2 = ledIndex + (indexModifier * columnDirection);
      mPixelGridConversionTable[index] = ledIndex2;
      indexModifier--;
     // Serial.println(ledIndex2);
    }
    columnDirection = -columnDirection;
  }   
}
void render() {
  for(uint16_t ledIndex=0; ledIndex<mPixelGridSize; ledIndex++) {
    mStrip->setPixelColor(ledIndex, mPixelBuffer[ledIndex]);
  }
 // mStrip->show();
}
uint16_t getIndexFromRowAndColumn(uint16_t row, uint16_t column) {
  uint16_t index = row * mNumColumns + column;
  return index;
}

void clear() {
  for(uint16_t row = 0; row < mNumRows; row++) {
    for(uint16_t column = 0; column < mNumColumns; column++) {
      setGridCellColour(row, column, mStrip->Color(0,0,0));
    }
  }
}

uint16_t numPixels() {
  return mPixelGridSize;
}

void setGridColour(uint32_t pColour) {
  for(uint16_t row = 0; row < mNumRows; row++) {
    for(uint16_t column = 0; column < mNumColumns; column++) {
      setGridCellColour(row, column, pColour);
    }
  }
}

uint32_t getGridCellColour(uint16_t row, uint16_t column) {
  uint16_t index = this->getIndexFromRowAndColumn(row, column);
  uint32_t colour = mPixelBuffer[index];
  return colour;
}

void setGridCellColour(uint16_t row, uint16_t column, uint32_t colour) {
  uint16_t index = this->getIndexFromRowAndColumn(row, column);
  uint16_t pixelIndex = mPixelGridConversionTable[index];
  mPixelBuffer[pixelIndex] = colour;
}
void setGridCellColour(uint16_t index, uint32_t colour) {  
  uint16_t pixelIndex = mPixelGridConversionTable[index];
  mPixelBuffer[pixelIndex] = colour;
}
};

