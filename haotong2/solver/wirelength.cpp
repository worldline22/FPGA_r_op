#include "solverObject.h"
#include "wirelength.h"
#include <set>

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
        std::cout << "Non-critical wirelength: " << nonCritWirelength << std::endl;
    } else {
        nonCritWirelength = 0;
    }  
    
    return cirtWirelength + nonCritWirelength;
}

int calculateInstanceWirelength(const SInstance& instance) {
    int totalWirelength = 0;
    std::set<int> net_IDs; // 使用 set 避免重复的 netID

    // 收集 netID
    for (const auto* inpin : instance.inpins) {
        if (inpin && inpin->netID != 0) {
            net_IDs.insert(inpin->netID);
        }
    }

    for (const auto* outpin : instance.outpins) {
        if (outpin && outpin->netID != 0) {
            net_IDs.insert(outpin->netID);
        }
    }

    // 计算线长
    for (int netID : net_IDs) {
        if (netID >= 0 && netID < NetArray.size()) {
            totalWirelength += calculateWirelength(*NetArray[netID]);
        }
    }

    return totalWirelength;
}



// int calculateWirelengthIncrease(const SInstance& instance_old, std::tuple<int, int, int> newLocation){
//     std::vector<SNet*> nets_old, nets_new;

//     SInstance instance_new{instance_old};
//     instance_new.Location = newLocation;

//     for (auto* inpin : instance_new.inpins) {
//         inpin->instanceOwner = &instance_new;
//     }

//     for (auto* outpin : instance_new.outpins) {
//         outpin->instanceOwner = &instance_new;
//     }

//     for (const auto* inpin : instance_old.inpins) {
//         if (inpin->netID == 0) {
//             continue;
//         }
//         nets_old.push_back(NetArray[inpin->netID]);
//         SNet net_new{*NetArray[inpin->netID]};
//         for (auto* outpin : net_new.outpins) {
//             outpin->instanceOwner = &instance_new;
//         }
//         nets_new.push_back(&net_new);
//     }

//     for (const auto* outpin : instance_old.outpins) {
//         if (outpin->netID == 0) {
//             continue;
//         }
//         nets_old.push_back(NetArray[outpin->netID]);
//         SNet net_new{*NetArray[outpin->netID]};
//         net_new.inpin->instanceOwner = &instance_new;
//         nets_new.push_back(&net_new);
//     }
        
//     int old_wirelength{}, new_wirelength{};
//     for (size_t i = 0; i < nets_old.size(); i++) {
//         old_wirelength += calculateWirelength(*nets_old[i]);
//         new_wirelength += calculateWirelength(*nets_new[i]);
//     }
//     return new_wirelength - old_wirelength;
// }

int num = 10;

int calculateWirelengthIncrease(SInstance* instance_old_pointer, std::tuple<int, int, int> newLocation) {
    std::vector<SNet> nets_old, nets_new;
    SInstance instance_old = *instance_old_pointer;
    int length1 = calculateInstanceWirelength(instance_old);
    SInstance instance_new; // 拷贝构造
    instance_new.fixed = instance_old.fixed;
    instance_new.Lib = instance_old.Lib;
    instance_new.id = instance_old.id;
    instance_new.baseLocation = instance_old.baseLocation;
    instance_new.Location = newLocation;
    instance_new.bank = instance_old.bank;
    instance_new.conn = instance_old.conn;
    instance_new.numMov = instance_old.numMov;
    
    for (auto* pin : instance_old.inpins) {
        if (pin) {
            // instance_new.inpins.push_back(new SPin(*pin));  // deep copy
            SPin pin_new{}, pin_old{*pin};
            pin_new.pinID = pin_old.pinID;
            pin_new.netID = pin_old.netID;
            pin_new.prop = pin_old.prop;
            pin_new.timingCritical = pin_old.timingCritical;
            pin_new.instanceOwner = &instance_new;
            instance_new.inpins.push_back(std::move(&pin_new));
        }
    }
    for (auto* pin : instance_old.outpins) {
        if (pin) {
            // instance_new.outpins.push_back(new SPin(*pin));  // deep copy
            SPin pin_new{}, pin_old{*pin};
            pin_new.pinID = pin_old.pinID;
            pin_new.netID = pin_old.netID;
            pin_new.prop = pin_old.prop;
            pin_new.timingCritical = pin_old.timingCritical;
            pin_new.instanceOwner = &instance_new;
            instance_new.outpins.push_back(std::move(&pin_new));
        }
    }

    // for (auto* inpin : instance_new.inpins) {
    //     inpin->instanceOwner = &instance_new;
    // }

    // for (auto* outpin : instance_new.outpins) {
    //     outpin->instanceOwner = &instance_new;
    // }
    
    int length2 = calculateInstanceWirelength(instance_old);

    for (const auto* inpin : instance_old.inpins) {
        if (inpin->netID == 0 || inpin->netID >= NetArray.size()) {
            continue;
        }
        nets_old.push_back(*NetArray[inpin->netID]); // 直接存储对象
        SNet net_new(*NetArray[inpin->netID]);
        for (auto* outpin : net_new.outpins) {
            outpin->instanceOwner = &instance_new;
        }
        nets_new.push_back(std::move(net_new));
    }

    int length3 = calculateInstanceWirelength(instance_old);

    for (const auto* outpin : instance_old.outpins) {
        if (outpin->netID == 0 || outpin->netID >= NetArray.size()) {
            continue;
        }
        nets_old.push_back(*NetArray[outpin->netID]); // 直接存储对象
        SNet net_new(*NetArray[outpin->netID]);
        net_new.inpin->instanceOwner = &instance_new;
        nets_new.push_back(std::move(net_new));
    }

    int length4 = calculateInstanceWirelength(instance_old);

    int old_wirelength = 0, new_wirelength = 0;
    for (size_t i = 0; i < nets_old.size(); i++) {
        old_wirelength += calculateWirelength(nets_old[i]);
        new_wirelength += calculateWirelength(nets_new[i]);
    }
    int length5 = calculateInstanceWirelength(instance_old);
    std::cout << "Old wirelength: " << old_wirelength << ", length1: " << length1 << ", length2: " << length2 << ", length3: " << length3 << ", length4: " << length4 << ", length5: " << length5;
    std::cout << ", New wirelength: " << new_wirelength << std::endl;
    if(num < 0){
        exit(0);
    }
    num--;
    return length1 - old_wirelength;
}
