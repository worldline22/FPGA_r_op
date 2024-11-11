# include "legal.h"
# include "global.h"
# include "solverObject.h"

bool checkControlSet() {    
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