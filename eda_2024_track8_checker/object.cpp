#include <iostream>
#include <sstream>
#include <iomanip>
#include <set>
#include "global.h"
#include "object.h"
#include "rsmt.h"
#include "util.h"

Tile::~Tile() {
  for (auto& pair : instanceMap) {
    for (auto& slot : pair.second) {
      delete slot;
    }
  }
  instanceMap.clear();
}

bool Tile::matchType(const std::string& modelType) {  
  std::string matchType = modelType;
  if ( modelType == "SEQ"   ||
      modelType == "LUT6"   || 
      modelType == "LUT5"   || 
      modelType == "LUT4"   || 
      modelType == "LUT3"   || 
      modelType == "LUT2"   || 
      modelType == "LUT1"   ||
      modelType == "LUT6X"  ||
      modelType == "DRAM"   ||        
      modelType == "CARRY4" ||
      modelType == "F7MUX"  ||
      modelType == "F8MUX" ) {
    matchType = "PLB";        
  }
  return tileTypes.find(matchType) != tileTypes.end();
}

bool Tile::addInstance(int instID, int offset, std::string modelType, const bool isBaseline) {
  if (matchType(modelType) == false) {        
    std::cout << "Error: " << getLocStr() << " " << modelType <<" instance " << instID << ", type mismatch with tile type" << std::endl;
    return false;
  }

  std::string mtp = unifyModelType(modelType);

  auto mapIter = instanceMap.find(mtp);
  if (mapIter == instanceMap.end()) {
    std::cout << "Error: Invalid slot type " << mtp << " @ " << getLocStr() << std::endl;
    return false;
  }

  if (offset >= (int)mapIter->second.size()) {
    std::cout << "Error: " << mtp << " slot offset " << offset << " exceeds the capacity" << std::endl;
    return false;
  }

  if (isBaseline){
    mapIter->second[offset]->addBaselineInstance(instID);
  } else {
    mapIter->second[offset]->addOptimizedInstance(instID);
  }
  return true;
}

void Tile::clearInstances() {
  for (auto& pair : instanceMap) {
    for (auto& slot : pair.second) {
      slot->clearInstances();
    }
  }
}

void Tile::clearBaselineInstances() {
  for (auto& pair : instanceMap) {
    for (auto& slot : pair.second) {
      slot->clearBaselineInstances();
    }
  }
}

void Tile::clearOptimizedInstances() {
  for (auto& pair : instanceMap) {
    for (auto& slot : pair.second) {
      slot->clearOptimizedInstances();
    }
  }
}

slotArr* Tile::getInstanceByType (std::string type) {
  auto mapIter = instanceMap.find(type);
  if (mapIter == instanceMap.end()) {
    return nullptr;
  }
  return &(mapIter->second);
}

void Tile::reportTile() {
  // report tile occupation
  std::string typeStr;
  for (auto type : tileTypes) {
    typeStr += type + " ";
  }
  std::cout << "  Tile " << getLocStr() << " type: " << typeStr << std::endl;
  
  std::cout << "  " << std::string(55, '-') << std::endl;
  std::cout << "  " << std::left << std::setw(15) << "Slot";
  std::cout << std::setw(32) << "| Occupied(baseline, optimized)";
  std::cout << std::setw(15) << "| Total" << std::endl;
  std::cout << "  " << std::string(55, '-') << std::endl;

  for (auto& pair : instanceMap) {
    std::pair<int, int> occupiedSlotCnt = std::make_pair(0, 0);
    for (unsigned int i = 0; i < pair.second.size(); i++) {
      if (pair.second[i]->getBaselineInstances().size() > 0) {
        occupiedSlotCnt.first++;
      }
      if (pair.second[i]->getOptimizedInstances().size() > 0) {
        occupiedSlotCnt.second++;
      }
    }  
    std::cout <<"  " << std::left << std::setw(15) << pair.first;
    std::string printStr = "( " + std::to_string(occupiedSlotCnt.first) + ", " + std::to_string(occupiedSlotCnt.second) + " )";
    std::cout <<"| " << std::setw(30) << printStr;
    std::cout <<"| " << std::setw(13) << pair.second.size() << std::endl;        
  }
  
  std::cout << "  " << std::string(55, '-') << std::endl;
  std::cout << std::endl;

  // more detailed information w.r.t occupation
  for (auto& pair : instanceMap) {    
    for (unsigned int i = 0; i < pair.second.size(); i++) {
      if (pair.second[i]->getBaselineInstances().size() == 0 && 
          pair.second[i]->getOptimizedInstances().size() == 0) {
        continue;
      }
      std::cout << "  " << pair.first << " #" << i << " (baseline, optimized)"<< std::endl;      

      std::list<int> baselineInstArr = pair.second[i]->getBaselineInstances();
      std::list<int> optimizedInstArr = pair.second[i]->getOptimizedInstances();

      // print two columns
      // left one is baseline instances
      // right one is optimized instances
      // if two list with different size, print "-" for empty item
      auto baselineIt = baselineInstArr.begin();
      auto optimizedIt = optimizedInstArr.begin();
      while (baselineIt != baselineInstArr.end() || optimizedIt != optimizedInstArr.end()) {
        std::cout << "    ";
        
        // Print baseline instance
        if (baselineIt != baselineInstArr.end()) {
          int instID = *baselineIt;
          if (glbInstMap.find(instID) != glbInstMap.end()) {
            Instance* instPtr = glbInstMap[instID];
            std::cout << std::left << std::setw(20) << ("inst_" + std::to_string(instID) + " " + instPtr->getModelName());
          } else {
            std::cout << std::left << std::setw(20) << "Error: Instance not found";
          }
          ++baselineIt;
        } else {
          std::cout << std::setw(20) << "-";
        }
        
        std::cout << " | ";
        
        // Print optimized instance
        if (optimizedIt != optimizedInstArr.end()) {
          int instID = *optimizedIt;
          if (glbInstMap.find(instID) != glbInstMap.end()) {
            Instance* instPtr = glbInstMap[instID];
            std::cout << std::left << std::setw(30) << ("inst_" + std::to_string(instID) + " " + instPtr->getModelName());
          } else {
            std::cout << std::left << std::setw(30) << "Error: Instance not found";
          }
          ++optimizedIt;
        } else {
          std::cout << std::left << std::setw(20) << "-";
        }
        
        std::cout << std::endl;
      }
    }
  }
  std::cout << std::endl;

  // report pin utilization
  std::set<int> baselineInpinSet = getConnectedLutSeqInput(true);
  std::set<int> optimizedInpinSet = getConnectedLutSeqInput(false);  

  std::cout << "  Detailed pin utilization:" << std::endl;
  std::cout << "    Input nets: Baseline " << baselineInpinSet.size() << "/" << MAX_TILE_PIN_INPUT_COUNT;
  std::cout << ", Optimized " << optimizedInpinSet.size() << "/" << MAX_TILE_PIN_INPUT_COUNT << std::endl;
   
  // print left baseline netID, right optimized netID, padding with "-" is not exist
  auto baselineNetIt = baselineInpinSet.begin();
  auto optimizedNetIt = optimizedInpinSet.begin();
  while (baselineNetIt != baselineInpinSet.end() || optimizedNetIt != optimizedInpinSet.end()) {
    std::cout << "      ";
    if (baselineNetIt != baselineInpinSet.end()) {      
      std::cout << std::left << std::setw(15) << ("net_" + std::to_string(*baselineNetIt));
      ++baselineNetIt;
    } else {
      std::cout << std::left << std::setw(15) << "-";
    }
    std::cout << " | ";
    if (optimizedNetIt != optimizedInpinSet.end()) {
      std::cout << std::left << std::setw(15) << ("net_" + std::to_string(*optimizedNetIt));
      ++optimizedNetIt;
    } else {
      std::cout << std::left << std::setw(15) << "-";
    }
    std::cout << std::endl;
  }

  // print output pin utilization
  std::cout << std::endl;
  std::set<int> baselineOutpinSet = getConnectedLutSeqOutput(true);
  std::set<int> optimizedOutpinSet = getConnectedLutSeqOutput(false);

  std::cout << "  Detailed output pin utilization:" << std::endl;
  std::cout << "    Output nets: Baseline " << baselineOutpinSet.size() << "/" << MAX_TILE_PIN_OUTPUT_COUNT;
  std::cout << ", Optimized " << optimizedOutpinSet.size() << "/" << MAX_TILE_PIN_OUTPUT_COUNT << std::endl;
   
  // print left baseline netID, right optimized netID, padding with "-" is not exist
  baselineNetIt = baselineOutpinSet.begin();
  optimizedNetIt = optimizedOutpinSet.begin();
  while (baselineNetIt != baselineOutpinSet.end() || optimizedNetIt != optimizedOutpinSet.end()) {
    std::cout << "      ";
    if (baselineNetIt != baselineOutpinSet.end()) {      
      std::cout << std::left << std::setw(15) << ("net_" + std::to_string(*baselineNetIt));
      ++baselineNetIt;
    } else {
      std::cout << std::left << std::setw(15) << "-";
    }
    std::cout << " | ";
    if (optimizedNetIt != optimizedOutpinSet.end()) {
      std::cout << std::left << std::setw(15) << ("net_" + std::to_string(*optimizedNetIt));
      ++optimizedNetIt; 
    } else {
      std::cout << std::left << std::setw(15) << "-";
    }
    std::cout << std::endl;
  }

  // report control set
  std::cout << std::endl;
  std::cout << "  Detailed control set:" << std::endl;
  for (int bank = 0; bank < 2; bank++) {
    std::set<int> baselineClkNets;
    std::set<int> baselineCeNets;
    std::set<int> baselineSrNets;
    getControlSet(true, bank, baselineClkNets, baselineCeNets, baselineSrNets);   

    std::set<int> optimizedClkNets;
    std::set<int> optimizedCeNets;
    std::set<int> optimizedSrNets;
    getControlSet(false, bank, optimizedClkNets, optimizedCeNets, optimizedSrNets);   

    std::cout << "    Bank " << bank << std::endl;    
    if (baselineClkNets.size() > 0 || optimizedClkNets.size() > 0) {
      std::cout << "      Clock nets: baseline = " << baselineClkNets.size() << ", optimized = " << optimizedClkNets.size() << std::endl;
      auto baselineClkNetIt = baselineClkNets.begin();
      auto optimizedClkNetIt = optimizedClkNets.begin();
      while (baselineClkNetIt != baselineClkNets.end() || optimizedClkNetIt != optimizedClkNets.end()) {
        std::cout << "        ";
        if (baselineClkNetIt != baselineClkNets.end()) {
          std::cout << std::left << std::setw(15) << ("net_" + std::to_string(*baselineClkNetIt));
          ++baselineClkNetIt;
        } else {
          std::cout << std::left << std::setw(15) << "-";
        }        
        std::cout << " | ";

        if (optimizedClkNetIt != optimizedClkNets.end()) {
          std::cout << std::left << std::setw(15) << ("net_" + std::to_string(*optimizedClkNetIt));
          ++optimizedClkNetIt;
        } else {
          std::cout << std::left << std::setw(15) << "-";
        }
        std::cout << std::endl;
      }
    }    

    if (baselineSrNets.size() > 0 || optimizedSrNets.size() > 0) {
      std::cout << "      Reset nets: baseline = " << baselineSrNets.size() << ", optimized = " << optimizedSrNets.size() << std::endl;
      auto baselineSrNetIt = baselineSrNets.begin();
      auto optimizedSrNetIt = optimizedSrNets.begin();
      while (baselineSrNetIt != baselineSrNets.end() || optimizedSrNetIt != optimizedSrNets.end()) {
        std::cout << "        ";
        if (baselineSrNetIt != baselineSrNets.end()) {
          std::cout << std::left << std::setw(15) << ("net_" + std::to_string(*baselineSrNetIt));
          ++baselineSrNetIt;  
        } else {
          std::cout << std::left << std::setw(15) << "-";
        }
        std::cout << " | ";
        if (optimizedSrNetIt != optimizedSrNets.end()) {
          std::cout << std::left << std::setw(15) << ("net_" + std::to_string(*optimizedSrNetIt));
          ++optimizedSrNetIt;
        } else {
          std::cout << std::left << std::setw(15) << "-";
        }
        std::cout << std::endl;
      }
    }

    if (baselineCeNets.size() > 0 || optimizedCeNets.size() > 0) {
      std::cout << "      CE nets: baseline = " << baselineCeNets.size() << ", optimized = " << optimizedCeNets.size() << std::endl;
      auto baselineCeNetIt = baselineCeNets.begin();
      auto optimizedCeNetIt = optimizedCeNets.begin();
      while (baselineCeNetIt != baselineCeNets.end() || optimizedCeNetIt != optimizedCeNets.end()) {
        std::cout << "        ";
        if (baselineCeNetIt != baselineCeNets.end()) {
          std::cout << std::left << std::setw(15) << ("net_" + std::to_string(*baselineCeNetIt));
          ++baselineCeNetIt;
        } else {
          std::cout << std::left << std::setw(15) << "-";
        }
        std::cout << " | ";
        if (optimizedCeNetIt != optimizedCeNets.end()) {
          std::cout << std::left << std::setw(15) << ("net_" + std::to_string(*optimizedCeNetIt));
          ++optimizedCeNetIt;
        } else {
          std::cout << std::left << std::setw(15) << "-";
        }
        std::cout << std::endl;
      }
    }
    std::cout << std::endl;
  }
}

bool Tile::initTile(const std::string& tileType) {
  
  if (tileTypes.find(tileType) != tileTypes.end()) {
    std::cout << "Error: Slot already initialized with same type " << tileType << std::endl;
    return false;
  } else {
    tileTypes.insert(tileType);
  }
  
  if (tileType == "PLB") {
    std::vector<Slot*> tmpLutSlotArr;
    for (unsigned int i = 0; i < MAX_LUT_CAPACITY; i++) {
      Slot* slot = new Slot();
      tmpLutSlotArr.push_back(slot);   
    }
    instanceMap["LUT"] = tmpLutSlotArr;

    std::vector<Slot*> tmpDffSlotArr;
    for (unsigned int i = 0; i < MAX_DFF_CAPACITY; i++) {
      Slot* slot = new Slot();
      tmpDffSlotArr.push_back(slot);   
    }
    instanceMap["SEQ"] = tmpDffSlotArr;

    std::vector<Slot*> tmpCarrySlotArr;
    for (unsigned int i = 0; i < MAX_CARRY4_CAPACITY; i++) {
      Slot* slot = new Slot();
      tmpCarrySlotArr.push_back(slot);   
    }
    instanceMap["CARRY4"] = tmpCarrySlotArr;

    std::vector<Slot*> tmpF7SlotArr;
    for (unsigned int i = 0; i < MAX_F7_CAPACITY; i++) {
      Slot* slot = new Slot();
      tmpF7SlotArr.push_back(slot);   
    }
    instanceMap["F7MUX"] = tmpF7SlotArr;

    std::vector<Slot*> tmpF8SlotArr;
    for (unsigned int i = 0; i < MAX_F8_CAPACITY; i++) {
      Slot* slot = new Slot();
      tmpF8SlotArr.push_back(slot);   
    }
    instanceMap["F8MUX"] = tmpF8SlotArr;

    std::vector<Slot*> tmpDramSlotArr;
    for (unsigned int i = 0; i < MAX_DRAM_CAPACITY; i++) {
      Slot* slota = new Slot();
      tmpDramSlotArr.push_back(slota);   
    }
    instanceMap["DRAM"] = tmpDramSlotArr;

  } else if (tileType == "DSP") {
    std::vector<Slot*> tmpSlotArr;
    for (unsigned int i = 0; i < MAX_DSP_CAPACITY; i++) {
      Slot* slot = new Slot();
      tmpSlotArr.push_back(slot);   
    }
    instanceMap["DSP"] = tmpSlotArr;
  } else if (tileType == "RAMA") {
    std::vector<Slot*> tmpSlotArr;
    for (unsigned int i = 0; i < MAX_RAM_CAPACITY; i++) {
      Slot* slot = new Slot();
      tmpSlotArr.push_back(slot);   
    }
    instanceMap["RAMA"] = tmpSlotArr;
  } else if (tileType == "RAMB") {
    std::vector<Slot*> tmpSlotArr;
    for (unsigned int i = 0; i < MAX_RAM_CAPACITY; i++) {
      Slot* slot = new Slot();
      tmpSlotArr.push_back(slot);   
    }
    instanceMap["RAMB"] = tmpSlotArr;
  } else if (tileType == "IOA") {
    std::vector<Slot*> tmpSlotArr;
    for (unsigned int i = 0; i < MAX_IO_CAPACITY; i++) {
      Slot* slot = new Slot();
      tmpSlotArr.push_back(slot);   
    }
    instanceMap["IOA"] = tmpSlotArr;

  } else if (tileType == "IOB") {
    std::vector<Slot*> tmpSlotArr;
    for (unsigned int i = 0; i < MAX_IO_CAPACITY; i++) {
      Slot* slot = new Slot();
      tmpSlotArr.push_back(slot);   
    }
    instanceMap["IOB"] = tmpSlotArr;

  } else if (tileType == "GCLK") {
    std::vector<Slot*> tmpSlotArr;
    for (unsigned int i = 0; i < MAX_GCLK_CAPACITY; i++) {
      Slot* slot = new Slot();
      tmpSlotArr.push_back(slot);   
    }
    instanceMap["GCLK"] = tmpSlotArr;

  } else if (tileType == "IPPIN") {
    std::vector<Slot*> tmpSlotArr;
    for (unsigned int i = 0; i < MAX_IPPIN_CAPACITY; i++) {
      Slot* slot = new Slot();
      tmpSlotArr.push_back(slot);   
    }
    instanceMap["IPPIN"] = tmpSlotArr;
  } else if (tileType == "FIXED") {
  } else {
    std::cout << "Error: Invalid slot type " << tileType << std::endl;
  }    

  return true;
}

bool Tile::isEmpty(bool isBaseline) {
  for (auto& pair : instanceMap) {
    for (auto& slot : pair.second) {
      // skip ippin
      if(pair.first == "IPPIN"){
        continue;
      }
      if (isBaseline) {
        if (!slot->getBaselineInstances().empty()) {
          return false;
        }
      } else {
        if (!slot->getOptimizedInstances().empty()) {
          return false;
        }
      }
    }
  }
  return true;
}

std::set<int> Tile::getConnectedLutSeqInput(bool isBaseline) {    
  std::set<int> netSet;  // using set to merge identical nets
  if (matchType("PLB") == false) {
    return netSet;
  }

  for (auto mapIter : instanceMap) {
    std::string slotType = mapIter.first;

    if (slotType != "LUT" && slotType != "SEQ") {
      continue;
    }

    for (auto slot : mapIter.second) {
      std::list<int> instArr;
      if (isBaseline) {
        instArr = slot->getBaselineInstances();
      } else {
        instArr = slot->getOptimizedInstances();
      }
      for (auto instID : instArr) {
        if (glbInstMap.find(instID) == glbInstMap.end()) {
          std::cout << "Error: Instance ID " << instID << " not found in the global instance map" << std::endl;
          continue;
        }      
        Instance* instPtr = glbInstMap[instID];     

        int numInpins = instPtr->getNumInpins();
        for (int i = 0; i < numInpins; i++) {
          Pin* pin = instPtr->getInpin(i);
          // if (pin->getProp() != PIN_PROP_NONE) { 
          //   // skip DFF ctrl, clk, reset pins
          //   continue;
          // }
          int netID = pin->getNetID();                    
          if (netID < 0) {  // unconnected pin
            continue;
          }
                    
          if (glbNetMap.find(netID) == glbNetMap.end()) {
            std::cout << "Error: Net ID " << netID << " not found in the global net map" << std::endl;
            continue;
          }
          Net* netPtr = glbNetMap[netID];

          // check if driver is in the same tile
          // only count pins driven by nets from other tile
          if (netPtr->getInpin() != nullptr) {
            Instance* driverInstPtr = netPtr->getInpin()->getInstanceOwner();
            std::tuple<int, int, int> driverLoc;
            if (isBaseline) {
              driverLoc = driverInstPtr->getBaseLocation();
            } else {
              driverLoc = driverInstPtr->getLocation();
            }
            if (std::get<0>(driverLoc) == col && std::get<1>(driverLoc) == row) {
              continue;
            }
          }
          netSet.insert(netID);                
        }
      }
    }
  }
  return netSet;
}

std::set<int> Tile::getConnectedLutSeqOutput(bool isBaseline) {
  std::set<int> netSet;
  if (matchType("PLB") == false) {
    return netSet;
  }

  for (auto mapIter : instanceMap) {
    std::string slotType = mapIter.first;
    if (slotType != "LUT" && slotType != "SEQ") {
      continue;
    }

    for (auto slot : mapIter.second) {
      std::list<int> instArr;
      if (isBaseline) {
        instArr = slot->getBaselineInstances();
      } else {
        instArr = slot->getOptimizedInstances();
      }
      for (auto instID : instArr) {
        if (glbInstMap.find(instID) == glbInstMap.end()) {
          std::cout << "Error: Instance ID " << instID << " not found in the global instance map" << std::endl;
          continue;
        }      
        Instance* instPtr = glbInstMap[instID];     

        int numOutpins = instPtr->getNumOutpins();
        for (int i = 0; i < numOutpins; i++) {
          Pin* pin = instPtr->getOutpin(i);
          int netID = pin->getNetID();
          if (netID < 0) {  // unconnected pin
            continue;
          }
          
          if (glbNetMap.find(netID) == glbNetMap.end()) {
            std::cout << "Error: Net ID " << netID << " not found in the global net map" << std::endl;
            continue;
          }
          Net* netPtr = glbNetMap[netID];
          if (netPtr->isIntraTileNet(isBaseline)) {
            continue;
          }
          netSet.insert(netID);    
        }                
      }
    }
  }
  return netSet;
}

bool Tile::getControlSet(
  const bool isBaseline,
  const int bank,
  std::set<int> &clkNets,
  std::set<int> &ceNets,
  std::set<int> &srNets) {

  for (auto mapIter : instanceMap) {
    std::string slotType = mapIter.first;
    // in PLB, only SEQ has control pins
    if (slotType != "SEQ") {
      continue;
    }
    
    // DFF bank0: 0-7, bank1: 8-15
    int startIdx = 0;
    int endIdx = 15;
    if (bank == 0) {
      startIdx = 0;
      endIdx = 7;
    } else if (bank == 1) {
      startIdx = 8;
      endIdx = 15;
    } else {
      std::cout << "Error: Invalid bank ID " << bank << std::endl;
      return false;
    }

    for (int slotIdx = startIdx; slotIdx <= endIdx; slotIdx++) {        
      Slot* slotPtr = mapIter.second[slotIdx];
      std::list<int> instArr;
      if (isBaseline) {
        instArr = slotPtr->getBaselineInstances();
      } else {
        instArr = slotPtr->getOptimizedInstances();
      }
      for (auto instID : instArr) {
        if (glbInstMap.find(instID) == glbInstMap.end()) {
          std::cout << "Error: Instance ID " << instID << " not found in the global instance map" << std::endl;
          return false;
        }      
        Instance* instPtr = glbInstMap[instID];     

        int numInpins = instPtr->getNumInpins();
        for (int i = 0; i < numInpins; i++) {
          Pin* pin = instPtr->getInpin(i);
          int netID = pin->getNetID();
          if (netID >= 0 ) {
            PinProp prop = pin->getProp();
            if (prop == PIN_PROP_CE) {
              ceNets.insert(netID);
            } else if (prop == PIN_PROP_CLOCK) {
              clkNets.insert(netID);
            } else if (prop == PIN_PROP_RESET) {
              srNets.insert(netID);
            }
          }
        }   
        int numOutpins = instPtr->getNumOutpins();
        for (int i = 0; i < numOutpins; i++) {
          Pin* pin = instPtr->getOutpin(i);
          int netID = pin->getNetID();
          if (netID >= 0 ) {
            PinProp prop = pin->getProp();
            if (prop == PIN_PROP_CE) {
              ceNets.insert(netID);
            } else if (prop == PIN_PROP_CLOCK) {
              clkNets.insert(netID);
            } else if (prop == PIN_PROP_RESET) {
              srNets.insert(netID);
            }
          }
        }    
      }
    }    
  }    
  return true;
}

void ClockRegion::reportClockRegion() {
  std::cout << "  Clock region " << getLocStr() << " has " << clockNets.size() << " clock nets." << std::endl;
  for (auto netID : clockNets) {
    std::cout << "    net_" << netID << std::endl;
  }
}

Instance::~Instance() {
  for (auto& pin : inpins) {
    delete pin;
  }
  inpins.clear();
  for (auto& pin : outpins) {
    delete pin;
  }
  outpins.clear();
}

Instance::Instance() {
  cellLib = nullptr;
  fixed = false;
  setLocation(std::make_tuple(-1, -1, -1));
  setBaseLocation(std::make_tuple(-1, -1, -1));
}

bool Instance::isPlaced() {
  if (std::get<0>(location) == -1 || std::get<1>(location) == -1 || std::get<2>(location) == -1) {
    return false;
  } else {
    return true;
  }
}

bool Instance::isMoved() {
  if (std::get<0>(location) != std::get<0>(baseLocation) ||
      std::get<1>(location) != std::get<1>(baseLocation) ||
      std::get<2>(location) != std::get<2>(baseLocation)) {
    return true;
  } else {
    return false;
  }
}

void Instance::setCellLib(Lib* libPtr) {
  cellLib = libPtr;

  // create pins   
  createInpins();   
  createOutpins();
}

void Instance::createInpins() {
  if (cellLib == nullptr) {
    return;
  }
  for (int i = 0; i < cellLib->getNumInputs(); i++) {
    Pin* pin = new Pin();
    pin->setNetID(-1);
    pin->setInstanceOwner(this);
    pin->setProp(cellLib->getInputProp(i));
    inpins.push_back(pin);
  }
}

void Instance::createOutpins() {
  if (cellLib == nullptr) {
    return;
  }
  for (int i = 0; i < cellLib->getNumOutputs(); i++) {
    Pin* pin = new Pin();
    pin->setNetID(-1);
    pin->setInstanceOwner(this);
    pin->setProp(cellLib->getOutputProp(i));
    outpins.push_back(pin);
  }
}

bool Net::isIntraTileNet(bool isBaseline) {
  if (inpin == nullptr) {
    return false;
  }
  Instance* driverInstPtr = inpin->getInstanceOwner();
  std::tuple<int, int, int> driverLoc;
  if (isBaseline) {
    driverLoc = driverInstPtr->getBaseLocation();
  } else {
    driverLoc = driverInstPtr->getLocation();
  }

  for (const auto* outpin : getOutputPins()) {
    Instance* sinkInstPtr = outpin->getInstanceOwner();
    std::tuple<int, int, int> sinkLoc;
    if (isBaseline) {
      sinkLoc = sinkInstPtr->getBaseLocation();
    } else {
      sinkLoc = sinkInstPtr->getLocation();
    }
    if (std::get<0>(driverLoc) != std::get<0>(sinkLoc) || 
        std::get<1>(driverLoc) != std::get<1>(sinkLoc)) {
      return false;
    }
  }
  return true;
}

int Net::getNumPins() {
  if(inpin != nullptr) {
    return outputPins.size() + 1;
  } else {    
    return outputPins.size();
  }
}

// read net connections from netlist
bool Net::addConnection(std::string conn) {
  // inst_2 I1  
  std::istringstream iss(conn);
  std::vector<std::string> tokens;
  std::string token;
  while (iss >> token) {
    tokens.push_back(token);
  }

  if (tokens.size() != 2) {
    std::cout << "Error: Invalid connection format " << conn << std::endl;
    return false;
  }

  int errCnt = 0;
  Instance* instPtr = nullptr;
  Pin* pinPtr = nullptr;

  std::string instName = tokens[0];
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
    std::string pinName = tokens[1];
    underscorePos = pinName.find('_');
    if (underscorePos != std::string::npos) {
      std::string dirStr = pinName.substr(0, underscorePos);
      std::string pinIDStr = pinName.substr(underscorePos + 1);
      int pinID = std::stoi(pinIDStr);
      if (dirStr == "I") {
        pinPtr = instPtr->getInpin(pinID);
        pinPtr->setNetID(getId());
        addOutputPin(pinPtr);
      } else if (dirStr == "O") {
        pinPtr = instPtr->getOutpin(pinID);
        pinPtr->setNetID(getId());
        if (getInpin() != nullptr) {
          std::cout << "Error: Multiple drivers for net ID = " << getId() << std::endl;
          errCnt++;
        }
        setInpin(pinPtr);
      } else {
        std::cout << "Invalid pin name format: " << pinName << std::endl;
        errCnt++;
      }
    }    
  }

  if (errCnt > 0) {
    return false;
  } else {
    return true;
  }
}

int Net::getCritWireLength(bool isBaseline) {
  int wirelength = 0;
  const Pin* driverPin = getInpin();
  if (!driverPin) {
    return 0;  // Return 0 if there's no driver pin
  }

  std::tuple<int, int, int> driverLoc;
  if (isBaseline) {
    driverLoc = driverPin->getInstanceOwner()->getBaseLocation();
  } else {
    driverLoc = driverPin->getInstanceOwner()->getLocation();
  }

  std::set<std::pair<int, int>> mergedPinLocs;  // merge identical pin locations
  for (const auto* outpin : getOutputPins()) {
    if (!outpin->getTimingCritical()) {
      continue;
    }
    std::tuple<int, int, int> sinkLoc;
    if (isBaseline) {
      sinkLoc = outpin->getInstanceOwner()->getBaseLocation();
    } else {
      sinkLoc = outpin->getInstanceOwner()->getLocation();
    }
    mergedPinLocs.insert(std::make_pair(std::get<0>(sinkLoc), std::get<1>(sinkLoc)));
  }

  for (auto loc : mergedPinLocs) {
    wirelength += std::abs(std::get<0>(loc) - std::get<0>(driverLoc)) +
                  std::abs(std::get<1>(loc) - std::get<1>(driverLoc));
  }

  return wirelength;
}

void Net::getMergedNonCritPinLocs(bool isBaseline, std::vector<int>& xCoords, std::vector<int>& yCoords) {
  const Pin* driverPin = getInpin();
  if (!driverPin) {
    return;
  }
  std::set<std::pair<int, int>> rsmtPinLocs;
  
  std::tuple<int, int, int> driverLoc;
  if (isBaseline) {
    driverLoc = driverPin->getInstanceOwner()->getBaseLocation();
  } else {
    driverLoc = driverPin->getInstanceOwner()->getLocation();
  }
  rsmtPinLocs.insert(std::make_pair(std::get<0>(driverLoc), std::get<1>(driverLoc)));
  
  for (const auto* outpin : getOutputPins()) {
    if (outpin->getTimingCritical()) {
      continue;
    }
    std::tuple<int, int, int> sinkLoc;
    if (isBaseline) {
      sinkLoc = outpin->getInstanceOwner()->getBaseLocation();
    } else {
      sinkLoc = outpin->getInstanceOwner()->getLocation();
    }
    rsmtPinLocs.insert(std::make_pair(std::get<0>(sinkLoc), std::get<1>(sinkLoc)));
  }
  for(auto loc : rsmtPinLocs) {
    xCoords.push_back(loc.first);
    yCoords.push_back(loc.second);
  }
}

int Net::getNonCritWireLength(bool isBaseline) {
  const Pin* driverPin = getInpin();
  if (!driverPin) {
    return 0;  // Return 0 if there's no driver pin
  }

  std::vector<int> xCoords;
  std::vector<int> yCoords;
  getMergedNonCritPinLocs(isBaseline, xCoords, yCoords);

  if (xCoords.size() > 1) {
    Tree mst = rsmt.fltTree(xCoords, yCoords);
    return rsmt.wirelength(mst);
  } else {
    return 0;
  }  
}

bool Net::reportNet() {
  std::string propStr;
  if (isClock()) {
    propStr = "clock";
  }

  if (this->isIntraTileNet(false)) {  // report optimized net property
    if (propStr.empty()) {
      propStr = "intra_tile";
    } else {
      propStr += "| intra_tile";
    }
  }
  
  std::cout << "  net_" << id << " " << propStr << std::endl;

  int numNonCritFanoutPins = 0;
  int numCritFanoutPins = 0;
  for (const auto* outpin : getOutputPins()) {
    if (outpin->getTimingCritical()) {
      numCritFanoutPins++;
    } else{
      numNonCritFanoutPins++;
    }
  }
  std::cout << "    Number of critical fanout pins: " << numCritFanoutPins << std::endl;
  std::cout << "    Number of non-critical fanout pins: " << numNonCritFanoutPins << std::endl;  
  std::cout << std::endl;  
  std::cout << "    Critical wirelength: Baseline = " << getCritWireLength(true) << "; Optimized = " << getCritWireLength(false) << std::endl;  
  std::cout << "    Non-critical wirelength: Baseline = " << getNonCritWireLength(true) << "; Optimized = " << getNonCritWireLength(false) << std::endl;  
  std::cout << std::endl;
      
  // if (nonCritWirelength > 0) {

  //   std::vector<int> xCoords;
  //   std::vector<int> yCoords;
  //   getMergedNonCritPinLocs(xCoords, yCoords);

  //   std::cout << "    Net Rectilinear Steiner Minimum Tree with " << xCoords.size() << " merged locations." << std::endl;
  //   if (xCoords.size() > 1) {
  //     Tree mst = rsmt.fltTree(xCoords, yCoords);
  //     rsmt.printtree(mst);
  //   }
  // }

  return true;
}
