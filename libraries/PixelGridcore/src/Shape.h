#ifndef SHAPE_H

#define SHAPE_H

class Shape {
  private:
    String mName;
    uint32_t mColour;
    uint32_t * mCells;
    uint16_t mRows;
    uint16_t mColumns;
    uint16_t mNumCells;
    uint16_t mX;
    uint16_t mY;
  public :  
    Shape(String pName, uint32_t pColour, uint16_t pRows, uint16_t pColumns) {
      this->mName = pName;
      this->mColour = pColour;
      mRows = pRows;
      mColumns = pColumns;
      mNumCells = mRows * mColumns;
      mCells = new uint32_t[mNumCells];
      setPosition(0,0);
    }

    ~Shape() {
      delete[] mCells;
    }

  String getName() {
    return this->mName;
  }

  uint16_t getIndexFromRowAndColumn(uint16_t row, uint16_t column) {
    uint16_t index = row * mColumns + column;
    return index;
  }

  void setPosition(uint16_t pX, uint16_t pY) {
    mX = pX;
    mY = pY;
  }

  uint16_t getX() {
    return mX;
  }
  uint16_t getY() {
    return mY;
  }

  uint16_t numRows() {
    return mRows;
  }
  uint16_t numColumns() {
    return mColumns;
  }

  uint32_t getColour(uint16_t pRow, uint16_t pColumn) {
    uint16_t index = getIndexFromRowAndColumn(pRow, pColumn);
    return mCells[index];
  }
  bool setColour(uint16_t pRow, uint16_t pColumn, uint32_t pColour) {
    bool success = false;
    if(pRow >= 0 && pRow < mRows) {
      if(pColumn >= 0 && pColumn < mColumns) {
        uint16_t index = getIndexFromRowAndColumn(pRow, pColumn);
        mCells[index] = pColour;
        success = true;
      }
    }
    return success;
  }
 static Shape* makeI(uint32_t pColour){
    return new Shape("I", pColour, 2, 2); // light blue
  }
  static Shape* makeJ(uint32_t pColour){
    return new Shape("J", pColour, 2, 2); // dark blue
  }
  static Shape* makeL(uint32_t pColour){
    return new Shape("L", pColour, 2, 2); // orange
  }
  static Shape* makeO(uint32_t pColour){
    Shape* shape = new Shape("O",pColour , 2, 2); 
    shape->setColour(0,0,pColour);
    shape->setColour(0,1,pColour);
    shape->setColour(1,0,pColour);
    shape->setColour(1,1,pColour);
    return shape;
  }
  static Shape* makeS(uint32_t pColour){
    return new Shape("S", pColour, 2, 2); // green
  }
  static Shape* makeT(uint32_t pColour){
    return new Shape("T", pColour, 2, 2); // purple
  }
  static Shape* makeZ(uint32_t pColour){
    Shape* shape = new Shape("Z",pColour , 2, 2);
  }
  
};
#endif
