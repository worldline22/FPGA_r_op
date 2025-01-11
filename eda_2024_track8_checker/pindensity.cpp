#include <iomanip>
#include "global.h"
#include "object.h"
#include "pindensity.h"

bool reportPinDensity() {  
  int checkedTileCnt = 0;

  // 1) baseline
  std::multimap<double, Tile*> baselinePinDensityMap;  
  for (int i = 0; i < chip.getNumCol(); i++) {
      for (int j = 0; j < chip.getNumRow(); j++) {
          Tile* tile = chip.getTile(i, j);
          if (tile->matchType("PLB") == false) {
              continue;        
          }
          if (tile->isEmpty(true)) {  // baseline
            continue;
          }

          // baseline
          int numInterTileConn = tile->getConnectedLutSeqInput(true).size() + tile->getConnectedLutSeqOutput(true).size();          
          double ratio = (double)(numInterTileConn) / (MAX_TILE_PIN_INPUT_COUNT + MAX_TILE_PIN_OUTPUT_COUNT);
          baselinePinDensityMap.insert(std::pair<double, Tile*>(ratio, tile));            
          checkedTileCnt++;
      }
  }
  const int top5Pct = checkedTileCnt * 0.05;

  std::cout << "  Baseline: " << std::endl;
  std::cout << "    Checked pin density on " << checkedTileCnt <<" tiles; top 5% count = " << top5Pct << " tiles." << std::endl;
  
  int top5PctCnt = 0;
  double totalPct = 0.0;
  // print some statistics in table  
  std::cout << "    List of Top-10 Congested Tiles" << std::endl;  
  std::cout << "    " << lineBreaker << std::endl;

  std::cout << "    Location | Input  | Output | Pin Density %" << std::endl;
  const int printCnt = 10;  
  for (auto it = baselinePinDensityMap.rbegin(); it != baselinePinDensityMap.rend(); it++) {
      Tile* tile = it->second;
      double ratio = it->first * 100.0;                
      // convert ratio to percentage        
      if (top5PctCnt < top5Pct) {         

          totalPct += ratio;
          top5PctCnt++;

          if (top5PctCnt < printCnt) {
            std::set<int> inPinSet = tile->getConnectedLutSeqInput(true);
            std::set<int> outPinSet = tile->getConnectedLutSeqOutput(true);
            std::string locStr = tile->getLocStr();
            std::cout << "    " << std::left << std::setw(8) << locStr << " ";
            std::cout << "| " << std::left << std::setw(2) << inPinSet.size() << "/" << (int)MAX_TILE_PIN_INPUT_COUNT <<"  ";
            std::cout << "| " << std::left << std::setw(2) << outPinSet.size() << "/" << (int)MAX_TILE_PIN_OUTPUT_COUNT<<"  ";
            std::cout << "| " << std::left << std::setw(4) << ratio << "%" << std::endl;            
          } else if (top5PctCnt == printCnt) {
            std::cout << "    ..." << std::endl;
            std::cout << "    " << lineBreaker << std::endl;
          }
      } else {
          break;
      }
  }
  double avgPct = totalPct / top5Pct;
  std::cout << "    Baseline top 5% congested tiles (" << top5Pct << " tiles) avg. pin density: " << std::setprecision(2) << avgPct << "%" << std::endl;
  std::cout << std::endl;

  // 2) optimized
  checkedTileCnt = 0;
  std::multimap<double, Tile*> optimizedPinDensityMap;
    for (int i = 0; i < chip.getNumCol(); i++) {
      for (int j = 0; j < chip.getNumRow(); j++) {
          Tile* tile = chip.getTile(i, j);
          if (tile->matchType("PLB") == false) {
              continue;        
          }
          if (tile->isEmpty(false)) {  // optimized
            continue;
          }

          // optimized
          int numInterTileConn = tile->getConnectedLutSeqInput(false).size() + tile->getConnectedLutSeqOutput(false).size();          
          double ratio = (double)(numInterTileConn) / (MAX_TILE_PIN_INPUT_COUNT + MAX_TILE_PIN_OUTPUT_COUNT);
          optimizedPinDensityMap.insert(std::pair<double, Tile*>(ratio, tile));                      
          checkedTileCnt++;
      }
  }
  std::cout << "  Optimized: " << std::endl;
  std::cout << "    Checked pin density on " << checkedTileCnt <<" tiles." << std::endl;

  // reset counter
  top5PctCnt = 0;
  totalPct = 0.0;
  // print some statistics in table  
  std::cout << "    List of Top-10 Congested Tiles" << std::endl;  
  std::cout << "    " << lineBreaker << std::endl;

  std::cout << "    Location | Input  | Output | Pin Density %" << std::endl;  
  for (auto it = optimizedPinDensityMap.rbegin(); it != optimizedPinDensityMap.rend(); it++) {
      Tile* tile = it->second;
      double ratio = it->first * 100.0;                
      // convert ratio to percentage        
      if (top5PctCnt < top5Pct) {         

          totalPct += ratio;
          top5PctCnt++;

          if (top5PctCnt < printCnt) {
            // optimized density
            std::set<int> inPinSet = tile->getConnectedLutSeqInput(false);
            std::set<int> outPinSet = tile->getConnectedLutSeqOutput(false);
            std::string locStr = tile->getLocStr();
            std::cout << "    " << std::left << std::setw(8) << locStr << " ";
            std::cout << "| " << std::left << std::setw(2) << inPinSet.size() << "/" << (int)MAX_TILE_PIN_INPUT_COUNT <<"  ";
            std::cout << "| " << std::left << std::setw(2) << outPinSet.size() << "/" << (int)MAX_TILE_PIN_OUTPUT_COUNT<<"  ";
            std::cout << "| " << std::left << std::setw(4) << ratio << "%" << std::endl;            
          } else if (top5PctCnt == printCnt) {
            std::cout << "    ..." << std::endl;
            std::cout << "    " << lineBreaker << std::endl;
          }
      } else {
          break;
      }
  }
  avgPct = totalPct / top5Pct;
  std::cout << "    Optimized top 5% congested tiles(" << top5Pct << " tiles) avg. pin density: " << std::setprecision(2) << avgPct << "%" << std::endl;
  std::cout << std::endl;

  return true;      
}