#pragma once

#include <map>
#include"object.h"

class Arch {
    int numCol;
    int numRow;
    int numClockCol;
    int numClockRow;
    Tile*** tileArray;
    ClockRegion ***clockRegionArray;

public:
    // Constructor
    Arch() : numCol(0), numRow(0), numClockCol(0), numClockRow(0), tileArray(nullptr), clockRegionArray(nullptr) {
        // Add your constructor code here
    }

    // Destructor
    ~Arch(); 

    // Getter and setter for all data members
    int getNumCol() const { return numCol; }
    int getNumRow() const { return numRow; }
    int getNumClockCol() const { return numClockCol; }
    int getNumClockRow() const { return numClockRow; }
    
    void setNumCol(int value) { numCol = value; }
    void setNumRow(int value) { numRow = value; }
    void setNumClockCol(int value) { numClockCol = value; }
    void setNumClockRow(int value) { numClockRow = value;}

    Tile* getTile(int col, int row) {
        return tileArray[col][row];
    }
    ClockRegion* getClockRegion(int col, int row) {
        return clockRegionArray[col][row];
    }

    bool readArch(std::string sclFileName, std::string clkFileName);
    void reportArch();

    void cleanSlots();  // to load placement result 

    bool getClockRegionCoordinate(int instCol, int InstRow, int& clockCol, int& clockRow);

private:
    void createTileArray(int numRow, int numCol);
    void createClockRegionArray(int numRow, int numCol); 

    bool readSclFile(std::string sclFileName);
    bool readClkFile(std::string clkFileName);
};


