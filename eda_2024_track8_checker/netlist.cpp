#include <iomanip>
#include "netlist.h"
#include "global.h"
#include "util.h"

bool readInputTiming(const std::string& fileName) {
  std::ifstream inputFile(fileName);
  if (!inputFile.is_open()) {
    std::cout << "Failed to open file: " << fileName << std::endl;
    return false;
  }

  std::string line;    
  int errCnt = 0;
  while (std::getline(inputFile, line)) {        
    if (line.empty() || line[0] == '#') {
      continue;
    }

    std::istringstream iss(line);
    std::string instName, pinName;

    std::vector<std::string> tokens;
    std::string token;
    while (iss >> token) {
      tokens.push_back(token);
    }

    if (tokens.size() != 2) {
      std::cout << "Invalid format of " << line << std::endl;
      continue;
    }

    Instance* instPtr = nullptr;
    instName = tokens[0];
    size_t underscorePos = instName.find('_');
    if (underscorePos != std::string::npos) {
      std::string subStr = instName.substr(underscorePos + 1);
      // Convert the second substring to an integer
      int instID = std::stoi(subStr);
      if (glbInstMap.find(instID) != glbInstMap.end()) {
        instPtr = glbInstMap[instID];
      }
    }

    if (instPtr == nullptr) {
      std::cout << "Invalid instance name format: " << instName << std::endl;
      errCnt++;
    } else {
      Pin* pinPtr = nullptr;
      pinName = tokens[1];
      underscorePos = pinName.find('_');
      if (underscorePos != std::string::npos) {
        std::string dirStr = pinName.substr(0, underscorePos);
        std::string pinIDStr = pinName.substr(underscorePos + 1);
        int pinID = std::stoi(pinIDStr);
        if (dirStr == "I") {
          pinPtr = instPtr->getInpin(pinID);
        } else if (dirStr == "O") {
          pinPtr = instPtr->getOutpin(pinID);
        } else {
          std::cout << "Invalid pin name format: " << pinName << std::endl;
          errCnt++;
        }
        if (pinPtr == nullptr) {
          std::cout << "Invalid pin name: " << pinName << std::endl;
          errCnt++;
        } else {
          pinPtr->setTimingCritical(true);
        }
      } 
    }
  }    

  inputFile.close();
  
  if (errCnt > 0) {
    return false;
  } else {
    return true;
  }    
}

bool readInputNodes(const std::string& fileName) {
  // Implementation of readInputNetlist function
  std::ifstream inputFile(fileName);
  if (!inputFile.is_open()) {
    std::cout << "Failed to open file: " << fileName << std::endl;
    return false;
  }

  // clear all baseline instances in all tiles 
  for (int i = 0; i < chip.getNumCol(); i++) {
    for (int j = 0; j < chip.getNumRow(); j++) {
      Tile* tile = chip.getTile(i, j);
      // new input netlist is being read, clear all existing instances in the tile
      tile->clearInstances();   
    }
  }

  // Parse the location string to extract the coordinates
  std::regex locationRegex("X(\\d+)Y(\\d+)Z(\\d+)");

  std::string line;    
  int errCnt = 0;
  while (std::getline(inputFile, line)) {        
    if (line.empty() || line[0] == '#') {
      continue;
    }
    std::istringstream iss(line);
    std::string location, type, name;
    bool isFixed = false;
    
    std::vector<std::string> tokens;
    std::string token;
    while (iss >> token) {
      tokens.push_back(token);
    }
    
    location = tokens[0];
    type = tokens[1];
    name = tokens[2];
    if (tokens.size() == 4) {
      isFixed = (tokens[3] == "FIXED");
    } 

    std::smatch match;
    int x, y, z;
    if (std::regex_search(location, match, locationRegex)) {
      x = std::stoi(match[1]);
      y = std::stoi(match[2]);
      z = std::stoi(match[3]);
    } else {
      std::cout << "Error: Invalid location format: " << location << std::endl;
      errCnt++;
    }

    int instID = -1;
    size_t underscorePos = name.find('_');
    if (underscorePos != std::string::npos) {
      std::string subStr = name.substr(underscorePos + 1);
      // Convert the second substring to an integer
      instID = std::stoi(subStr);
    } else {
      std::cout << "Error: Invalid name format: " << name << std::endl;
      errCnt++;
    }
     
    // Check if the instance already exists in the map
    if (glbInstMap.find(instID) != glbInstMap.end()) {
      std::cout << "Error: Instance with name " << name << " already exists in the map." << std::endl;
      errCnt++;
      continue; // Skip adding the instance to the map
    }
    
    // Find the corresponding Lib object
    Lib* libPtr = nullptr;
    auto libIt = glbLibMap.find(type);
    if (libIt == glbLibMap.end()) {
      std::cout << "Error: Library with name " << type << " not found." << std::endl;
      errCnt++;
      continue; 
    } else {
      libPtr = libIt->second;
    }

    // Add the new instance object to the instMap
    Instance* newInstance = new Instance();
    newInstance->setInstanceName(name);
    newInstance->setModelName(type);
    newInstance->setBaseLocation(std::make_tuple(x, y, z));
    newInstance->setFixed(isFixed);
    newInstance->setCellLib(libPtr);
    glbInstMap[instID] = newInstance;

    // add the instance to the corresponding tile
    Tile* tilePtr = chip.getTile(x, y);
    if (tilePtr != nullptr) {
      // add baseline coordinate
      if (tilePtr->addInstance(instID, z, type, true) == false) {
        std::cout << "Error: Failed to add baseline coordinate for instance " << name << std::endl;
        return false;
      } 
    }
  }
  inputFile.close();

  if (errCnt > 0) {
    return false;
  } else {
    return true;
  }
}

bool readOutputNetlist(const std::string& fileName) {
  // Implementation of readInputNetlist function
  std::ifstream inputFile(fileName);
  if (!inputFile.is_open()) {
    std::cout << "Failed to open file: " << fileName << std::endl;
    return false;
  }

  // clear existing optimized instances in all tiles 
  for (int i = 0; i < chip.getNumCol(); i++) {
    for (int j = 0; j < chip.getNumRow(); j++) {
      Tile* tile = chip.getTile(i, j);      
      tile->clearOptimizedInstances();   
    }
  }

  // Parse the location string to extract the coordinates
  std::regex locationRegex("X(\\d+)Y(\\d+)Z(\\d+)");

  std::string line;
  int errCnt = 0;
  while (std::getline(inputFile, line)) {
    //lineCnt++;
    if (line.empty() || line[0] == '#') {
      continue;
    }

    std::istringstream iss(line);

    std::vector<std::string> tokens;
    std::string token;
    while (iss >> token) {
      tokens.push_back(token);
    }
    
    std::string location = tokens[0];
    std::string type = tokens[1];
    std::string name = tokens[2];

    int instID = -1;
    size_t underscorePos = name.find('_');
    if (underscorePos != std::string::npos) {
      std::string subStr = name.substr(underscorePos + 1);
      // Convert the second substring to an integer
      instID = std::stoi(subStr);
    } else {
      std::cout << "Error, Invalid name format: " << name << std::endl;
      errCnt++;
    }

    std::smatch match;
    int x, y, z;
    if (std::regex_search(location, match, locationRegex)) {
      x = std::stoi(match[1]);
      y = std::stoi(match[2]);
      z = std::stoi(match[3]);
    } else {
      std::cout << "Error, Invalid location format: " << location << std::endl;
      errCnt++;
    }

    // Check if the instance already exists in the map
    auto mIt = glbInstMap.find(instID);
    if (mIt == glbInstMap.end()) {
      std::cout << "Error, Instance with name " << name << " can not be indexed." << std::endl;
      errCnt++;
      continue; // Skip adding the instance to the map
    }
    // Add the new instance object to the instMap
    mIt->second->setLocation(std::make_tuple(x, y, z));

    Tile* tilePtr = chip.getTile(x, y);
    if (tilePtr != nullptr) {
      // add optimized coordinate
      if (tilePtr->addInstance(instID, z, type, false) == false) {
        std::cout << "Error: Failed to add optimized coordinate for instance " << name << std::endl;
        return false;
      } 
    }
  }
  inputFile.close();

  int totalCnt = glbInstMap.size();
  int fixedCnt = 0;
  int movableCnt = 0;
  int replacedFixedCnt = 0;
  int replacedMovableCnt = 0;
  for (const auto& pair : glbInstMap) {
    Instance* instance = pair.second;
    if (!instance->isPlaced()) {
      std::cout << "Error: instance " << instance->getInstanceName() << " is un-placed." << std::endl;
      errCnt++;
      continue;
    }    

    if (instance->isFixed()) {
      fixedCnt++;
      if (instance->isMoved()) {
        std::cout << "Error: fixed instance " << instance->getInstanceName() << " is moved." << std::endl;
        replacedFixedCnt++;
        errCnt++;
      } 
    } else {
      movableCnt++;
      if (instance->isMoved()) {
        replacedMovableCnt++;
      }
    }    
  }

  // Print table
  std::cout << "\n  Instance Statistics:\n";
  std::cout << "  -----------------------------------------------\n";
  std::cout << "  Category | Count     | Re-placed   | %Re-placed \n";
  std::cout << "  -----------------------------------------------\n";
  std::cout << "  Total    | " << std::setw(9) << totalCnt << " | -           | -          \n";
  std::cout << "  Fixed    | " << std::setw(9) << fixedCnt << " | " << std::setw(11) << replacedFixedCnt << " | " 
            << std::fixed << std::setprecision(1) << std::setw(9) 
            << (fixedCnt > 0 ? (100.0 * replacedFixedCnt / fixedCnt) : 0.0) << "% \n";
  std::cout << "  Movable  | " << std::setw(9) << movableCnt << " | " << std::setw(11) << replacedMovableCnt << " | " 
            << std::fixed << std::setprecision(1) << std::setw(9) 
            << (movableCnt > 0 ? (100.0 * replacedMovableCnt / movableCnt) : 0.0) << "% \n";
  std::cout << "  -----------------------------------------------\n";

  if (errCnt > 0) {
    return false;
  } else {
    return true;
  }
}

bool readInputNets(const std::string& fileName) {
  std::ifstream inputFile(fileName);
  if (!inputFile.is_open()) {
    std::cout << "Failed to open file: " << fileName << std::endl;
    return false;
  }

  int numErr = 0;
  std::string line;
  std::vector<std::string> netLines;
  bool isInsideNet = false;
  std::string netInfoStr;

  while (std::getline(inputFile, line)) {
    if (line.empty() || line[0] == '#') {
      continue;
    }
    if (line.substr(0, 3) == "net") {
      isInsideNet = true;
      netInfoStr = line.substr(4);
      netLines.clear();
    }
    if (isInsideNet) {
      netLines.push_back(line);
    }
    if (line.substr(0, 6) == "endnet") {
      isInsideNet = false;

      std::istringstream iss(netInfoStr);
      std::vector<std::string> tokens;
      std::string token;
      while (iss >> token) {
        tokens.push_back(token);
      }

      std::string netNameStr = tokens[0];
      int netID = -1;
      size_t underscorePos = netNameStr.find('_');
      if (underscorePos != std::string::npos) {
        std::string subStr = netNameStr.substr(underscorePos + 1);
        // Convert the second substring to an integer
        netID = std::stoi(subStr);
      } else {
        std::cout << "Invalid name format: " << netNameStr << std::endl;
      }

      unsigned int numPins = std::stoi(tokens[1]);
      if (netLines.size() != (numPins + 2)) {
        // The first line is the net name, the last line is the ending maker
        std::cout << "Wrong number of connections of net " << netNameStr << std::endl;
        continue;
      }

      // Create a new Net object
      Net* newNet = new Net(netID);
      unsigned int idx = 0;
      for (auto conn : netLines) {
        if (idx == 0 || idx == netLines.size() - 1) {
          // skip starting and ending lines 
          idx++;
          continue;
        }
        if (newNet->addConnection(conn) == false) {
          numErr++;
        }
        idx++;
      }
      if (tokens.size() == 3) {
        std::string clockStr = tokens[2];
        if (clockStr == "clock") {
          newNet->setClock(true);
        }
      } 
     
      // Add the new Net object to the netMap
      glbNetMap[netID] = newNet;
    }
  }
  inputFile.close();

  if (numErr > 0 ) {
    return false;
  } else {
    return true;
  }
}

bool reportDesignStatistics() {
  std::cout << "  Number of instances: " << glbInstMap.size() << std::endl;

  std::map<std::string, std::pair<int,int> > countByType;  // <total_cnt, fixed_cnt>
  for (auto inst : glbInstMap) {
    std::string modelName = inst.second->getModelName();
    modelName = unifyModelType(modelName);
    if (countByType.find(modelName) == countByType.end()) {
      if (inst.second->isFixed()) {
        countByType[modelName] = std::make_pair(1, 1);
      } else {
        countByType[modelName] = std::make_pair(1, 0);
      }
    } else {
      countByType[modelName].first++;
      if (inst.second->isFixed()) {
        countByType[modelName].second++;
      }
    }
  }

  // Print countByType as a table, type has fixed width of 20 characters
  std::cout << "  " << lineBreaker << std::endl;
  std::cout << "  " << std::left << std::setw(20) << "Type";
  std::cout << std::setw(12) << "| Total";
  std::cout << std::setw(12) << "| Fixed" << std::endl;
  std::cout << "  " << lineBreaker << std::endl;
  for (auto entry : countByType) {
    std::string modelName = entry.first;
    int totalCnt = entry.second.first;
    int fixedCnt = entry.second.second;
    std::cout <<"  " << std::left << std::setw(20) << modelName;
    std::cout <<"| " << std::setw(10) << totalCnt;
    std::cout <<"| " << std::setw(10) << fixedCnt << std::endl;
  }
  std::cout << "  " << lineBreaker << std::endl;
  std::cout << std::endl;

  std::cout << "  Number of nets: " << glbNetMap.size() << std::endl;
  // categorize nets by the number of pins
  std::map<std::string, int> netCountByGroup; // <group, count>
  int numIntraNet = 0;
  int numClkNet = 0;
  int numTotalPins = 0; 
  int numTotalCriticalPin = 0;  
  for (auto net : glbNetMap) {
    unsigned int numPins = net.second->getNumPins();
    numTotalPins += numPins;
    std::string group;
    if (numPins < 2) {
      group = "grp1: < 2 pins";
    } else if (numPins == 2) {
      group = "grp2: 2 pins";
    } else if (numPins >= 3 && numPins <= 10) {
      group = "grp3: 3~10 pins";
    } else if (numPins >= 11 && numPins <= 50) {
      group = "grp4: 11~50 pins";
    } else if (numPins >= 51 && numPins <= 100) {
      group = "grp5: 51~100 pins";
    } else if (numPins > 100 && numPins <= 1000) {
      group = "grp6: 101~1000 pins";
    } else {
      group = "grp7: >1000 pins";      
    }
    netCountByGroup[group]++;
    if (net.second->isIntraTileNet(true)) { 
      // report baseline net property
      numIntraNet++;
    }
    if (net.second->isClock()) {
      numClkNet++;
    }

    if (net.second->getInpin()->getTimingCritical()) {
      numTotalCriticalPin++;
    }
    std::list<Pin*> netOutPins = net.second->getOutputPins();
    for (auto oPin : netOutPins) {
      if (oPin->getTimingCritical()) {
        numTotalCriticalPin++;
      }
    }
  }
  if (netCountByGroup.find("grp1: < 2 pins") == netCountByGroup.end()) {
    netCountByGroup.emplace("grp1: < 2 pins", 0);
  }

  // Print netCountByGroup as a table
  std::cout << "  Number of clock nets: " << numClkNet << std::endl;  
  std::cout << "  " << lineBreaker << std::endl;
  std::cout << "  " << std::left << std::setw(22) << "Group";
  std::cout << std::setw(10) << "| Count" << std::endl;
  std::cout << "  " << lineBreaker << std::endl;
  for (auto entry : netCountByGroup) {
    std::string group = entry.first;
    int count = entry.second;
    std::cout <<"  " << std::left << std::setw(22) << group;
    std::cout <<"| " << std::setw(10) << count << std::endl;
  }
  std::cout << "  " << lineBreaker << std::endl;
  std::cout << std::endl;
  std::cout << "  " << numIntraNet << " out of " << glbNetMap.size() <<" are intra-tile nets."<< std::endl;
  std::cout << "  " << numTotalCriticalPin << " out of " << numTotalPins <<" are timing critical pins."<< std::endl;  
  return true;
}
