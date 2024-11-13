#include <map>
#include <set>
#include "../checker_legacy/global.h"
#include "../checker_legacy/arch.h"
#include <solverObject.h>

std::set<int> getConnectedLutSeqInput(){
    
}

int getConnectedLutSeqOutput(){

}

// bool matchType(const std::string& modelType) {  
//   std::string matchType = modelType;
//   if ( modelType == "SEQ"   ||
//       modelType == "LUT6"   || 
//       modelType == "LUT5"   || 
//       modelType == "LUT4"   || 
//       modelType == "LUT3"   || 
//       modelType == "LUT2"   || 
//       modelType == "LUT1"   ||
//       modelType == "LUT6X"  ||
//       modelType == "DRAM"   ||        
//       modelType == "CARRY4" ||
//       modelType == "F7MUX"  ||
//       modelType == "F8MUX" ) {
//     matchType = "PLB";        
//   }
//   return tileTypes.find(matchType) != tileTypes.end();
// }

int calculatePinDensity() {
    int checkedTileCnt = 0;

    std::multimap<int, int> pinDensity;
    for (int i = 0; i < chip.numCol; i++) {
        for (int j = 0; j < chip.nunRow; j++) {
            STile* tile = chip.getTile(i, j);
            // if (tile->matchType("PLB") == false) {
            //     continue;        
            // }
            // if (tile->isEmpty(true)) {  // baseline
            //     continue;
            // }

            // baseline
            int numInterTileConn = tile->getConnectedLutSeqInput(true).size() + tile->getConnectedLutSeqOutput(true).size();          
            double ratio = (double)(numInterTileConn) / (MAX_TILE_PIN_INPUT_COUNT + MAX_TILE_PIN_OUTPUT_COUNT);
            baselinePinDensityMap.insert(std::pair<double, Tile*>(ratio, tile));            
            checkedTileCnt++;
        }
    }
}

// 

// for (tileptr : TileArray) {
//     if tileptr->type != 1 continue;
//     tileptr->pindensity = ... 
//     for instance in bank:
//         for net in instance:
// }