#include "solverObject.h"
#include "global.h"
#include "rsmt.h"
#include <cassert>
#include <fstream>

void getMergedNonCritPinLocs(const SNet& net, std::vector<int>& xCoords, std::vector<int>& yCoords) { // increase the passing parameter net
    const SPin* driverPin = net.inpin; // change type from Pin to SPin, change getInpin() to net.inpin
    if (!driverPin) {
        return;
    }
    std::set<std::pair<int, int>> rsmtPinLocs;
    std::tuple<int, int, int> driverLoc;

    // remove baseline case from the code base
    driverLoc = driverPin->instanceOwner->Location;

    rsmtPinLocs.insert(std::make_pair(std::get<0>(driverLoc), std::get<1>(driverLoc)));
    
    for (const auto* outpin : net.outpins) { // change type from Pin to SPin, change getOutpins() to net.outpins
        if (outpin->timingCritical) {
        continue;
        }
        std::tuple<int, int, int> sinkLoc;
        sinkLoc = outpin->instanceOwner->Location;
        rsmtPinLocs.insert(std::make_pair(std::get<0>(sinkLoc), std::get<1>(sinkLoc)));
    }
    for(auto loc : rsmtPinLocs) {
        xCoords.push_back(loc.first);
        yCoords.push_back(loc.second);
    }
}


int calculateWirelength(const SNet& net) {
    // calculate the critical wirelength of the net
    int cirtWirelength = 0;
    const SPin* driverPin = net.inpin;
    if (!driverPin) {
        cirtWirelength = 0;  // Return 0 if there's no driver pin
    }

    std::tuple<int, int, int> driverLoc;
    driverLoc = driverPin->instanceOwner->Location;

    std::set<std::pair<int, int>> mergedPinLocs;  // merge identical pin locations
    for (const auto* outpin : net.outpins) {
        if (!outpin->timingCritical) {
        continue;
        }
        std::tuple<int, int, int> sinkLoc;
        sinkLoc = outpin->instanceOwner->Location;
       
        mergedPinLocs.insert(std::make_pair(std::get<0>(sinkLoc), std::get<1>(sinkLoc)));
    }

    for (auto loc : mergedPinLocs) {
        cirtWirelength += std::abs(std::get<0>(loc) - std::get<0>(driverLoc)) +
                    std::abs(std::get<1>(loc) - std::get<1>(driverLoc));
    }


    // calcaulte the non-critical wirelength of the net
    int nonCritWirelength = 0;
    std::vector<int> xCoords;
    std::vector<int> yCoords;
    getMergedNonCritPinLocs(net, xCoords, yCoords); // declared in sovlverObject.h; defined in solverObject.cpp

    if (xCoords.size() > 1) {
        Tree mst = rsmt.fltTree(xCoords, yCoords); // defined in rsmt.h struct Tree
        nonCritWirelength = rsmt.wirelength(mst);
    } else {
        nonCritWirelength = 0;
    }  
    
    return cirtWirelength + nonCritWirelength;
}