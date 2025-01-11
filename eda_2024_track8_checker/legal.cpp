#include <iomanip>
#include "legal.h"
#include "global.h"
#include "object.h"

bool legalCheck() {
 
  int numErrors = 0;
  std::cout << "  1.1 Check instance location and tile capacity." << std::endl;
  if (checkTypeAndCapacity(true) == false) {
    numErrors++;
  } else {
    std::cout << "        Baseline placement passed capacity check." << std::endl;
  }  
  if (checkTypeAndCapacity(false) == false) {
    numErrors++;
  } else {
    std::cout << "        Optimized placement passed capacity check." << std::endl;
  }

  std::cout << "  1.2 Check control set constraint." << std::endl;
  std::cout << "        Baseline placement:" << std::endl;
  if (checkControlSet(true) == false) {
    std::cout << "        Baseline placement passed control set check." << std::endl;
    numErrors++;
  }
  std::cout << "        Optimized placement:" << std::endl;
  if (checkControlSet(false) == false) {
    std::cout << "        Optimized placement passed control set check." << std::endl;
    numErrors++;
  }

  std::cout << "  1.3 Check clock region constraint." << std::endl;
  std::cout << "        Baseline placement:" << std::endl;
  if (checkClockRegion(true) == false) {
    numErrors++;
  } 
  std::cout << "        Optimized placement:" << std::endl;
  if (checkClockRegion(false) == false) {
    numErrors++;
  } 

  if (numErrors > 0) {
    std::cout << "  LegalCheck failed with " << numErrors << " errors." << std::endl;
    return false;
  } else {
    std::cout << "  Legalization check passed." << std::endl;
    return true;
  }
}

bool checkTypeAndCapacity(bool isBaseline) {
  int errorCount = 0;

  // check tile capacity
  int overflowTileCount = 0;
  for (int i = 0; i < chip.getNumCol(); i++) {
    for (int j = 0; j < chip.getNumRow(); j++) {

      Tile* tile = chip.getTile(i, j);            
      
      std::list<std::pair<std::string, int> > overflow;
      for (auto mapIter = tile->getInstanceMapBegin(); mapIter != tile->getInstanceMapEnd(); mapIter++) {
        std::string modelType = mapIter->first;        
        slotArr slots = mapIter->second;
        for (int idx = 0; idx < (int)slots.size(); idx++) {
          Slot* slot = slots[idx];
          if (slot == nullptr) {
            continue;
          }
          // check if the slot is legally occupied
          std::list<int> instances;
          if (isBaseline) {
            instances = slot->getBaselineInstances();
          } else {
            instances = slot->getOptimizedInstances();
          }
          if (instances.size() > 1) {
            // 1) 2-LUTs are allowed but total number of input should not exceed 6
            if (modelType == "LUT") {
              if (instances.size() > 2) {
                overflow.push_back(std::pair<std::string, int>(modelType, idx));                        
              } else {
                std::set<int> totalInputs;
                for (auto instID : instances) {
                  Instance* instPtr = glbInstMap.find(instID)->second;
                  std::vector<Pin*> inpins = instPtr->getInpins();
                  for (auto pin : inpins) {
                    if (pin->getNetID() != -1) {
                      totalInputs.insert(pin->getNetID());
                    }
                  }                                    
                }
                if (totalInputs.size() > 6) {
                  overflow.push_back(std::pair<std::string, int>(modelType, idx));
                }
              }
            } else {
              overflow.push_back(std::pair<std::string, int>(modelType, idx));                        
            }
          } else {
            // check DRAM and lut
            if (modelType == "DRAM") {
              if (isBaseline && slot->getBaselineInstances().empty()) {
                continue;
              } else if (!isBaseline && slot->getOptimizedInstances().empty()) {
                continue;
              }
              // DRAM at slot0 blocks lut slot 0~3
              // DRAM at slot1 blocks lut slot 4~7
              slotArr* lutSlotArr = tile->getInstanceByType("LUT");
              if (idx == 0) {
                for (int lutIdx = 0; lutIdx < 4; lutIdx++) {
                  Slot* lutSlot = (*lutSlotArr)[lutIdx];
                  if (isBaseline && lutSlot->getBaselineInstances().size() > 0) {
                    overflow.push_back(std::pair<std::string, int>("LUT-DRAM", lutIdx));
                  } else if (!isBaseline && lutSlot->getOptimizedInstances().size() > 0) {
                    overflow.push_back(std::pair<std::string, int>("LUT-DRAM", lutIdx));
                  }
                }
              } else if (idx == 1) {
                for (int lutIdx = 4; lutIdx < 8; lutIdx++) {
                  Slot* lutSlot = (*lutSlotArr)[lutIdx];
                  if (isBaseline && lutSlot->getBaselineInstances().size() > 0) {
                    overflow.push_back(std::pair<std::string, int>("LUT-DRAM", lutIdx));
                  } else if (!isBaseline && lutSlot->getOptimizedInstances().size() > 0) {
                    overflow.push_back(std::pair<std::string, int>("LUT-DRAM", lutIdx));
                  }
                }
              } else {
                // dram with invalid slot index
              }
            }
          }
        }  // end for each slot
      }  // end for each slot type

      // print error
      if (overflow.empty() == false) {
        std::cout << "Error: Tile " << tile->getLocStr() << " is over the capacity." << std::endl;
        for (auto pair : overflow) {
          std::cout << "  Slot type: " << pair.first << " slot index: " << pair.second << std::endl;
        }
        overflowTileCount++;
      }            
    } // end for each row
  } // end for each column

  if (errorCount > 0) {
    return false;
  } else {
    return true;
  }    
}

bool checkControlSet(bool isBaseline) {    
  // Return true if the control set is valid, otherwise return false
  int errorCount = 0;

  int tileCount = 0;
  std::map<int, int> tileClkCount;
  std::map<int, int> tileCeCount;
  std::map<int, int> tileResetCount;
  for (int i = 0; i < chip.getNumCol(); i++) {
    for (int j = 0; j < chip.getNumRow(); j++) {
      Tile* tile = chip.getTile(i, j);
      if (tile->matchType("PLB") == false) {
        continue;        
      }
      tileCount++;

      std::set<int> plbClkNets;
      std::set<int> plbCeNets;
      std::set<int> plbResetNets;
      for (int bank = 0; bank < 2; bank++) {
        std::set<int> clkNets;
        std::set<int> ceNets;
        std::set<int> srNets;
        if (tile->getControlSet(isBaseline, bank, clkNets, ceNets, srNets) == false) {             
          errorCount++;
        }

        int numClk = clkNets.size();      
        int numReset = srNets.size();    
        int numCe = ceNets.size();

        if (numClk > MAX_TILE_CLOCK_PER_PLB_BANK) {
          std::cout << "Error: Multiple clock nets in bank " << bank << " of tile " << tile->getLocStr() << std::endl;
          errorCount++;
        }
        if (numReset > MAX_TILE_RESET_PER_PLB_BANK) {
          std::cout << "Error: Multiple reset nets in bank " << bank << " of tile " << tile->getLocStr() << std::endl;
          errorCount++;
        }  
        if (numCe > MAX_TILE_CE_PER_PLB_BANK) {
          std::cout << "Error: Multiple CE nets in bank " << bank << " of tile " << tile->getLocStr() << std::endl;        
          errorCount++;
        }  

        // merge control sets in different banks
        plbClkNets.insert(clkNets.begin(), clkNets.end());
        plbCeNets.insert(ceNets.begin(), ceNets.end());
        plbResetNets.insert(srNets.begin(), srNets.end());
      }

      int plbCeCount = (int)plbCeNets.size();
      int plbClkCount = (int)plbClkNets.size();
      int plbResetCount = (int)plbResetNets.size();

      if (tileCeCount.find(plbCeCount) == tileCeCount.end()) {
        tileCeCount[plbCeCount] = 1;
      } else {
        tileCeCount[plbCeCount]++;
      } 
      if (tileClkCount.find(plbClkCount) == tileClkCount.end()) {
        tileClkCount[plbClkCount] = 1;
      } else {
        tileClkCount[plbClkCount]++;
      }
      if (tileResetCount.find(plbResetCount) == tileResetCount.end()) {
        tileResetCount[plbResetCount] = 1;
      } else {
        tileResetCount[plbResetCount]++;
      }
    }
  }

  // print stat in table format
  std::cout << "          Checked control set on " << tileCount << " tiles." << std::endl;
  std::cout << "          Control Set Statistics(tile count v.s number of control nets):" << std::endl;
  std::cout << "          ---------------------------------------" << std::endl;
  std::cout << "          |       |  0  |  1  |  2  |  3  |  4  |" << std::endl;
  std::cout << "          ---------------------------------------" << std::endl;

  auto printRow = [&](const std::string& label, const std::map<int, int>& countMap) {
    std::cout << "          | " << std::left << std::setw(5) << label << " |";
    for (int i = 0; i <= 4; ++i) {
      int count = countMap.count(i) ? countMap.at(i) : 0;
      std::cout << std::setw(5) << count << "|";
    }
    std::cout << std::endl;
  };

  printRow("Clock", tileClkCount);
  printRow("Reset", tileResetCount);
  printRow("CE", tileCeCount);
  std::cout << "          ---------------------------------------" << std::endl;

  if (errorCount > 0) {
    return false;
  } else {
    return true;
  }   
}

bool checkClockRegion(bool isBaseline) {    
  // Return true if the clock region is valid, otherwise return false
  int errorCount = 0;
  
  // clean up 
  for (int j = chip.getNumClockRow() - 1; j >=0 ; j--) {    
    for (int i = 0; i < chip.getNumClockCol(); i++) {
      ClockRegion* clockRegion = chip.getClockRegion(i, j);
      clockRegion->clearClockNets();
    }
  }

  for (auto inst : glbInstMap) {
    int instCol;
    int instRow;
    if (isBaseline) {
      instCol  = std::get<0>(inst.second->getBaseLocation());
      instRow  = std::get<1>(inst.second->getBaseLocation());
    } else {
      instCol  = std::get<0>(inst.second->getLocation());
      instRow  = std::get<1>(inst.second->getLocation());
    }    
    int clockCol = -1;
    int clockRow = -1;
    if (chip.getClockRegionCoordinate(instCol, instRow, clockCol, clockRow) == false) {
      std::cout << "Error: Instance " << inst.second->getInstanceName() << " is not in any clock region." << std::endl;
      errorCount++;
      continue;
    }

    ClockRegion* clockRegion = chip.getClockRegion(clockCol, clockRow);
    // check each pin of the instance
    for (int idx = 0; idx < inst.second->getNumInpins(); idx++) {
      Pin* pin = inst.second->getInpin(idx);
      int netID = pin->getNetID();
      if (pin->getProp() == PIN_PROP_CLOCK && netID != -1) { // connected clock pin
        Net* netPtr = glbNetMap.find(netID)->second;
        if (netPtr->isClock()) {
          clockRegion->addClockNet(pin->getNetID());   
        }
      }
    }

    // check each pin of the instance
    for (int idx = 0; idx < inst.second->getNumOutpins(); idx++) {
      Pin* pin = inst.second->getOutpin(idx);
      int netID = pin->getNetID();
      if (pin->getProp() == PIN_PROP_CLOCK && netID != -1) { // connected clock pin
        Net* netPtr = glbNetMap.find(netID)->second;
        if (netPtr->isClock()) {
          clockRegion->addClockNet(pin->getNetID());   
        }
      }
    }
  }

  // report clock region
  int overflowRegionCount = 0;
  for (int j = chip.getNumClockRow() - 1; j >=0 ; j--) {
    std::cout << "          | ";
    for (int i = 0; i < chip.getNumClockCol(); i++) {
      ClockRegion* clockRegion = chip.getClockRegion(i, j);
      std::cout << std::left << std::setw(2) << clockRegion->getNumClockNets() <<"| ";
      if (clockRegion->getNumClockNets() > MAX_REGION_CLOCK_COUNT) {
        overflowRegionCount++;
      }
    }
    std::cout << std::endl;
  }

  if (overflowRegionCount > 0) {    
    std::cout << "Error: " << overflowRegionCount << " clock regions have more than " << MAX_REGION_CLOCK_COUNT << " clock nets." << std::endl;
    errorCount++;
  } else {
    std::cout << "          All clock regions passed legal check." << std::endl;
  }    

  if (errorCount > 0) {
    return false;
  } else {
    return true;
  }   
}

void reportClockRegion(const int col, const int row) {

  std::cout << "  Baseline:" << std::endl;
  checkClockRegion(true);  // report baseline placement
  // report a specific clock region
  ClockRegion* clockRegion = chip.getClockRegion(col, row);
  if (clockRegion) {
    clockRegion->reportClockRegion();  // report optimized placement
  }
  std::cout << std::endl;

  std::cout << "  Optimized:" << std::endl;
  checkClockRegion(false);  // report optimized placement
  // report a specific clock region
  clockRegion = chip.getClockRegion(col, row);
  if (clockRegion) {
    clockRegion->reportClockRegion();  // report optimized placement
  }  
}
