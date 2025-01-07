#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include "global.h"
#include "arch.h"

Arch::~Arch() {
  // Implementation of destructor
  if (tileArray != nullptr) {
    for (int i = 0; i < numCol; i++) {
      for (int j = 0; j < numRow; j++) {
        delete tileArray[i][j];
      }
      delete[] tileArray[i];
    }
    delete[] tileArray;
  }

  if (clockRegionArray != nullptr) {
    for (int i = 0; i < numClockCol; i++) {
      for (int j = 0; j < numClockRow; j++) {
        delete clockRegionArray[i][j];
      }
      delete[] clockRegionArray[i];
    }
    delete[] clockRegionArray;
  }
}

void Arch::createTileArray(int numCol, int numRow) {
  tileArray = new Tile**[numCol];
  for (int i = 0; i < numCol; i++) {
    tileArray[i] = new Tile*[numRow];
    for (int j = 0; j < numRow; j++) {
      tileArray[i][j] = new Tile(i, j);
    }
  }
}

void Arch::createClockRegionArray(int numCol, int numRow) {
  clockRegionArray = new ClockRegion**[numCol];
  for (int i = 0; i < numCol; i++) {
    clockRegionArray[i] = new ClockRegion*[numRow];
    for (int j = 0; j < numRow; j++) {
      clockRegionArray[i][j] = new ClockRegion();
    }
  }
}

// translate the coordinate of the instance to the coordinate of the clock region
bool Arch::getClockRegionCoordinate(int instCol, int InstRow, int& clockCol, int& clockRow) {
  if (instCol < 0 || instCol >= numCol || InstRow < 0 || InstRow >= numRow) {
    return false;
  }

  for (int i = 0; i < numClockCol; i++) {
    for (int j = 0; j < numClockRow; j++) {
      ClockRegion* clockRegion = getClockRegion(i, j);
      if (instCol >= clockRegion->getXLeft() && instCol <= clockRegion->getXRight() &&
          InstRow >= clockRegion->getYBottom() && InstRow <= clockRegion->getYTop()) {
        clockCol = i;
        clockRow = j;
        return true;
      }
    }
  }

  clockCol = -1;
  clockRow = -1;
  return false;  
}


bool Arch::readSclFile(std::string sclFileName) {
  // Implementation of readSclFile function
  std::ifstream sclFile(sclFileName);
  if (!sclFile.is_open()) {
    std::cout << "Failed to open file: " << sclFileName << std::endl;
    return false;
  }

  std::string line;
  unsigned int numErr = 0;
  while (std::getline(sclFile, line)) {
    if (line.empty() || line[0] == '#') {
      continue;
    }

    if (getNumCol() == 0) {
      // process the header line
      if (line.substr(0, 7) == "SITEMAP") {
        std::istringstream iss(line.substr(7));
        int numRow, numCol;
        if (iss >> numCol >> numRow) {
          createTileArray(numCol, numRow);
          setNumCol(numCol);
          setNumRow(numRow);
        } else {
          std::cout << "Failed to parse SITEMAP line: " << line << std::endl;
          return false;
        }
        continue;
      }           
    } 

    // process the rest of the file
    std::istringstream iss(line);
    std::string location, modelType;
    if (iss >> location >> modelType) {
      int x, y;
      if (std::sscanf(location.c_str(), "X%dY%d", &x, &y) == 2) {
        Tile* tile = getTile(x, y);
        if (tile->initTile(modelType) == false) {
          numErr++;
        }
      } else {
        std::cout << "Failed to parse location string: " << location << std::endl;
        numErr++;
      }
    } else {
      if (line == "END_SITEMAP") {
        break;
      }
      std::cout << "Failed to parse site line: " << line << std::endl;
      numErr++;
    }
  }

  // init fixed tile
  for (int i = 0; i < numCol; i++) {
    for (int j = 0; j < numRow; j++) {
      Tile* tile = getTile(i, j);
      if (tile->getNumTileTypes() == 0) {
        tile->addType("UNDEFINED");
      }
    }
  }

  if (numErr > 0) {
    return false;
  } else {
    return true;
  }
}

bool Arch::readClkFile(std::string clkFileName) {
  // Implementation of readClkFile function
  std::ifstream clkFile(clkFileName);
  if (!clkFile.is_open()) {
    std::cout << "Failed to open file: " << clkFileName << std::endl;
    return false;
  }

  std::string line;
  while (std::getline(clkFile, line)) {
    if (line.empty() || line[0] == '#') {
      continue;
    }

    if (getNumClockCol() == 0) {
      // process the header line
      if (line.substr(0, 12) == "CLOCKREGIONS") {
        std::istringstream iss(line.substr(12));
        int numClockRow, numClockCol;
        if (iss >> numClockCol >> numClockRow) {
          createClockRegionArray(numClockCol, numClockRow);
          setNumClockCol(numClockCol);
          setNumClockRow(numClockRow);
        } else {
          std::cout << "Failed to parse CLOCKREGION line: " << line << std::endl;
          return false;
        }
        continue;
      }
    }
    
    // process the rest of the file
    std::istringstream iss(line);
    std::string location;
    int left, right, bottom, top;
    if (iss >> location >> left >> right >> bottom >> top) {
      int x, y;
      if (std::sscanf(location.c_str(), "X%dY%d", &x, &y) == 2) {
        ClockRegion* clockRegion = getClockRegion(x, y);
        clockRegion->setBoundingBox(left, right, bottom, top);
      } else {
        std::cout << "Failed to parse location string: " << location << std::endl;
        return false;
      }
    } else {
      if (line == "END_CLOCKREGIONS") {
        break;
      }
      std::cout << "Failed to parse clock region line: " << line << std::endl;
      return false;
    }
  }
  return true;
}

bool Arch::readArch(std::string sclFileName, std::string clkFileName) {
  // Implementation of readArch function
  if (readSclFile(sclFileName) == false) {
    std::cout << "Failed to read SCL file: " << sclFileName << std::endl;
    return false;
  }

  if (readClkFile(clkFileName) == false) {
    std::cout << "Failed to read CLK file: " << clkFileName << std::endl;
    return false;
  }

  return true;
}


void Arch::cleanSlots() {
  // Implementation of cleanSlots function
  for (int i = 0; i < numCol; i++) {
    for (int j = 0; j < numRow; j++) {
      //tileArray[i][j]->clearInstances();
    }
  }
}

void Arch::reportArch() {
  // Implementation of reportArch function
  std::cout << "  Number of columns: " << numCol << std::endl;
  std::cout << "  Number of rows: " << numRow << std::endl;
  std::cout << "  Number of clock regions: " << numClockCol * numClockRow << std::endl;   

  std::map<std::string, int> tileCountByType;
  for (int i = 0 ; i < numCol; i++) {
    for (int j = 0; j < numRow; j++) {
      Tile* tile = getTile(i, j);
      std::set<std::string> tileTypes = tile->getTileTypes();
      for (const std::string& type : tileTypes) {
        if (tileCountByType.find(type) == tileCountByType.end()) {
          tileCountByType[type] = 1;
        } else {
          tileCountByType[type]++;
        }            
      }
    }
  }

  std::multimap<int, std::string> sortedTileCount;
  for (const auto& pair : tileCountByType) {
    sortedTileCount.insert(std::make_pair(pair.second, pair.first));
  }

  std::cout << std::endl;
  std::cout << "  Tile Count by Type:" << std::endl;
  std::cout << "  " << lineBreaker << std::endl;
  std::cout << "  " << std::left << std::setw(20) << "Type" << "| Count" << std::endl;
  std::cout << "  " << lineBreaker << std::endl;
  for (const auto& pair : sortedTileCount) {
    if (pair.second == "UNDEFINED") {
      continue;
    }
    std::cout <<"  "<< std::left << std::setw(20) << pair.second << "| " << pair.first << std::endl;
  }
  std::cout << "  " << lineBreaker << std::endl;
}
