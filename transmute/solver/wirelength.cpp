#include "solverObject.h"
#include "wirelength.h"

RecSteinerMinTree rsmt;

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
    std::cout << "Pin inserted" << std::endl;

    for (auto loc : mergedPinLocs) {
        cirtWirelength += std::abs(std::get<0>(loc) - std::get<0>(driverLoc)) +
                    std::abs(std::get<1>(loc) - std::get<1>(driverLoc));
        std::cout << "Critical wirelength: " << cirtWirelength << std::endl;
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

int calculateInstanceWirelength(const SInstance& instance) {
    int totalWirelength = 0;
    std::vector<int> net_IDs;
    for (const auto* inpin : instance.inpins) {
        if (inpin->netID == 0) {
            continue;
        }
        net_IDs.push_back(inpin->netID);
    }

    for (const auto* outpin : instance.outpins) {
        if (outpin->netID == 0) {
            continue;
        }
        net_IDs.push_back(outpin->netID);
    }

    for (int netID : net_IDs) {
        totalWirelength += calculateWirelength(*NetArray[netID]);
    }
    return totalWirelength;
}

int calculateWirelengthIncrease(const SInstance& instance_old, std::tuple<int, int, int> newLocation){
    std::vector<int> net_IDs;
    std::vector<int> pin_IDs;
    std::vector<SNet*> nets_old, nets_new;

    SInstance instance_new{instance_old};
    instance_new.Location = newLocation;

    for (auto* inpin : instance_new.inpins) {
        inpin->instanceOwner = &instance_new;
    }

    for (auto* outpin : instance_new.outpins) {
        outpin->instanceOwner = &instance_new;
    }

    for (const auto* inpin : instance_old.inpins) {
        if (inpin->netID == 0) {
            continue;
        }
        nets_old.push_back(NetArray[inpin->netID]);
        SNet net_new{*NetArray[inpin->netID]};
        for (auto* outpin : net_new.outpins) {
            outpin->instanceOwner = &instance_new;
        }
        nets_new.push_back(&net_new);
    }

    for (const auto* outpin : instance_old.outpins) {
        if (outpin->netID == 0) {
            continue;
        }
        nets_old.push_back(NetArray[outpin->netID]);
        SNet net_new{*NetArray[outpin->netID]};
        net_new.inpin->instanceOwner = &instance_new;
        nets_new.push_back(&net_new);
    }
        
    int old_wirelength{}, new_wirelength{};
    for (size_t i = 0; i < nets_old.size(); i++) {
        old_wirelength += calculateWirelength(*nets_old[i]);
        new_wirelength += calculateWirelength(*nets_new[i]);
    }
    return new_wirelength - old_wirelength;
}