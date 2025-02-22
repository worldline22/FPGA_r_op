#include "solverObject.h"
#include "wirelength.h"

RecSteinerMinTree rsmt;

// void getMergedNonCritPinLocs(const SNet& net, std::vector<int>& xCoords, std::vector<int>& yCoords) { // increase the passing parameter net
//     const SPin* driverPin = net.inpin; // change type from Pin to SPin, change getInpin() to net.inpin
//     if (!driverPin) {
//         return;
//     }
//     std::set<std::pair<int, int>> rsmtPinLocs;
//     std::tuple<int, int, int> driverLoc;

//     // remove baseline case from the code base
//     driverLoc = driverPin->instanceOwner->Location;

//     rsmtPinLocs.insert(std::make_pair(std::get<0>(driverLoc), std::get<1>(driverLoc)));
    
//     for (const auto* outpin : net.outpins) { // change type from Pin to SPin, change getOutpins() to net.outpins
//         if (outpin->timingCritical) {
//         continue;
//         }
//         std::tuple<int, int, int> sinkLoc;
//         sinkLoc = outpin->instanceOwner->Location;
//         rsmtPinLocs.insert(std::make_pair(std::get<0>(sinkLoc), std::get<1>(sinkLoc)));
//     }
//     for(auto loc : rsmtPinLocs) {
//         xCoords.push_back(loc.first);
//         yCoords.push_back(loc.second);
//     }
// }


// int calculateWirelength(const SNet& net) {
//     // calculate the critical wirelength of the net
//     int cirtWirelength = 0;
//     const SPin* driverPin = net.inpin;
//     if (!driverPin) {
//         cirtWirelength = 0;  // Return 0 if there's no driver pin
//     }

//     std::tuple<int, int, int> driverLoc;
//     driverLoc = driverPin->instanceOwner->Location;

//     std::set<std::pair<int, int>> mergedPinLocs;  // merge identical pin locations
//     for (const auto* outpin : net.outpins) {
//         if (!outpin->timingCritical) {
//         continue;
//         }
//         std::tuple<int, int, int> sinkLoc;
//         sinkLoc = outpin->instanceOwner->Location;
       
//         mergedPinLocs.insert(std::make_pair(std::get<0>(sinkLoc), std::get<1>(sinkLoc)));
//     }
//     std::cout << "Pin inserted" << std::endl;

//     for (auto loc : mergedPinLocs) {
//         cirtWirelength += std::abs(std::get<0>(loc) - std::get<0>(driverLoc)) +
//                     std::abs(std::get<1>(loc) - std::get<1>(driverLoc));
//         std::cout << "Critical wirelength: " << cirtWirelength << std::endl;
//     }


//     // calcaulte the non-critical wirelength of the net
//     int nonCritWirelength = 0;
//     std::vector<int> xCoords;
//     std::vector<int> yCoords;
//     getMergedNonCritPinLocs(net, xCoords, yCoords); // declared in sovlverObject.h; defined in solverObject.cpp

//     if (xCoords.size() > 1) {
//         Tree mst = rsmt.fltTree(xCoords, yCoords); // defined in rsmt.h struct Tree
//         nonCritWirelength = rsmt.wirelength(mst);
//     } else {
//         nonCritWirelength = 0;
//     }  
    
//     return cirtWirelength + nonCritWirelength;
// }

// int calculateInstanceWirelength(const SInstance& instance) {
//     int totalWirelength = 0;
//     std::vector<int> net_IDs;
//     for (const auto* inpin : instance.inpins) {
//         if (inpin->netID == 0) {
//             continue;
//         }
//         net_IDs.push_back(inpin->netID);
//     }

//     for (const auto* outpin : instance.outpins) {
//         if (outpin->netID == 0) {
//             continue;
//         }
//         net_IDs.push_back(outpin->netID);
//     }

//     for (int netID : net_IDs) {
//         totalWirelength += calculateWirelength(*NetArray[netID]);
//     }
//     return totalWirelength;
// }

// int calculateWirelengthIncrease(const SInstance& instance_old, std::tuple<int, int, int> newLocation){
//     std::vector<int> net_IDs;
//     std::vector<int> pin_IDs;
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

int calculate_WL_Increase(SInstance* inst_old_ptr, std::tuple<int, int, int> newLoc) {
    // calculate the previous wirelength
    int result = 0;
    // std::cout << "inst_old_ptr->id: " << inst_old_ptr->id << std::endl;
    // if (inst_old_ptr->id == 1970) std::cout << "newLoc: " << std::get<0>(newLoc) << " " << std::get<1>(newLoc) << " " << std::get<2>(newLoc) << std::endl;
    std::set<int> netID_set;
    for (auto inpin : inst_old_ptr->inpins) {
        if (inpin->netID != -1) {
            if (NetArray[inpin->netID]->clock) continue;
            if (NetArray[inpin->netID]->outpins.size() > 9900) continue;
            netID_set.insert(inpin->netID);
        }
    }
    for (auto outpin : inst_old_ptr->outpins) {
        if (outpin->netID != -1) {
            if (NetArray[outpin->netID]->clock) continue;
            if (NetArray[outpin->netID]->outpins.size() > 9900) continue;
            netID_set.insert(outpin->netID);
        }
    }
    for (int netID : netID_set) {
        // std::cout << "netID: " << netID << std::endl;
        assert(netID >= 0 && netID < int(NetArray.size()));
        const SNet* net = NetArray[netID];

        // crit wirelength of this net
        int crit_prev = 0;
        int crit_new = 0;
        const SPin* driverPin = net->inpin;
        if (!driverPin) continue;

        std::tuple<int, int, int> driverLoc = driverPin->instanceOwner->Location;
        // Don't forget to modify the driver location!
        std::tuple<int, int, int> driverLoc_new;
        if (driverPin->instanceOwner == inst_old_ptr) {
            driverLoc_new = newLoc;
        }
        else {
            driverLoc_new = driverLoc;
        }
        std::set<std::pair<int, int>> mergedPinLocs_prev{};
        std::set<std::pair<int, int>> mergedPinLocs_new{};
        for (auto outpin : net->outpins) {
            if (!outpin->timingCritical) continue;
            std::tuple<int, int, int> sinkLoc = outpin->instanceOwner->Location;
            mergedPinLocs_prev.insert(std::make_pair(std::get<0>(sinkLoc), std::get<1>(sinkLoc)));
            if (outpin->instanceOwner == inst_old_ptr) {
                assert(outpin->instanceOwner->id == inst_old_ptr->id);
                mergedPinLocs_new.insert(std::make_pair(std::get<0>(newLoc), std::get<1>(newLoc)));
            }
            else {
                mergedPinLocs_new.insert(std::make_pair(std::get<0>(sinkLoc), std::get<1>(sinkLoc)));
            }
        }
        for (auto loc : mergedPinLocs_prev) {
            crit_prev += std::abs(std::get<0>(loc) - std::get<0>(driverLoc)) + std::abs(std::get<1>(loc) - std::get<1>(driverLoc));
        }
        for (auto loc : mergedPinLocs_new) {
            crit_new += std::abs(std::get<0>(loc) - std::get<0>(driverLoc_new)) + std::abs(std::get<1>(loc) - std::get<1>(driverLoc_new));
        }
        // 更优化地，可以直接只放入更新的pin
        result += (crit_new - crit_prev) * 2;
        // if (inst_old_ptr->id == 1970) std::cout << "crit_new: " << crit_new << " crit_prev: " << crit_prev << std::endl;

        ////////////////////////////////////////////////////////////////////////
        // non-crit wirelength of this net
        int noncrit_prev = 0;
        int noncrit_new = 0;
        std::set<std::pair<int, int>> rsmtPinLocs_prev{};
        std::set<std::pair<int, int>> rsmtPinLocs_new{};
        rsmtPinLocs_prev.insert(std::make_pair(std::get<0>(driverLoc), std::get<1>(driverLoc)));
        rsmtPinLocs_new.insert(std::make_pair(std::get<0>(driverLoc_new), std::get<1>(driverLoc_new)));
        for (auto outpin : net->outpins) {
            if (outpin->timingCritical) continue;
            std::tuple<int, int, int> sinkLoc = outpin->instanceOwner->Location;
            rsmtPinLocs_prev.insert(std::make_pair(std::get<0>(sinkLoc), std::get<1>(sinkLoc)));
            if (outpin->instanceOwner == inst_old_ptr) {
                assert(outpin->instanceOwner->id == inst_old_ptr->id);
                rsmtPinLocs_new.insert(std::make_pair(std::get<0>(newLoc), std::get<1>(newLoc)));
            }
            else {
                rsmtPinLocs_new.insert(std::make_pair(std::get<0>(sinkLoc), std::get<1>(sinkLoc)));
            }
        }
        std::vector<int> xCoords_prev, yCoords_prev;
        std::vector<int> xCoords_new, yCoords_new;
        for (auto loc : rsmtPinLocs_prev) {
            xCoords_prev.push_back(loc.first);
            yCoords_prev.push_back(loc.second);
        }
        for (auto loc : rsmtPinLocs_new) {
            xCoords_new.push_back(loc.first);
            yCoords_new.push_back(loc.second);
        }
        if (xCoords_prev.size() > 1) {
            Tree mst_prev = rsmt.fltTree(xCoords_prev, yCoords_prev);
            noncrit_prev = rsmt.wirelength(mst_prev);
        }
        if (xCoords_new.size() > 1) {
            Tree mst_new = rsmt.fltTree(xCoords_new, yCoords_new);
            noncrit_new = rsmt.wirelength(mst_new);
        }
        result += noncrit_new - noncrit_prev;
        // if (inst_old_ptr->id == 1970) std::cout << "noncrit_new: " << noncrit_new << " noncrit_prev: " << noncrit_prev << std::endl;
    }
    return result;
}

int calculate_bank_WL_Increase(STile* tile_old_ptr, bool oldbank, std::pair<int, int> newLoc) {
    int result = 0;
    std::set<SInstance*> inst_set{};
    if (oldbank == false) {
        for (int ii = 0; ii < 4; ++ii)
        {
            //// 考虑将其单独做为数据结构
            for (auto InstID : tile_old_ptr->instanceMap["LUT"][ii].current_InstIDs)
            {
                assert(InstID >= 0 && InstID < int(InstArray.size()));
                assert(InstID != 0);
                inst_set.insert(InstArray[InstID]);
            }
        }
        for (int ii = 0; ii < 8; ++ii)
        {
            for (auto InstID : tile_old_ptr->instanceMap["SEQ"][ii].current_InstIDs)
            {
                assert(InstID >= 0 && InstID < int(InstArray.size()));
                assert(InstID != 0);
                inst_set.insert(InstArray[InstID]);
            }
        }
        if (inst_set.size() == 0) return 0;
        // std::cout << "inst_set size: " << inst_set.size() << std::endl;
        ////////////////////////////////////
        for (int i = 0; i < (int)tile_old_ptr->netsConnected_bank0.size(); i++){
            SNet *net = NetArray[tile_old_ptr->netsConnected_bank0[i]];
            if (net->clock) continue;
            if (net->outpins.size() > 9900) continue;

            // crit
            int crit_prev = 0;
            int crit_new = 0;
            const SPin* driverPin = net->inpin;
            if (!driverPin) continue;

            std::tuple<int, int, int> driverLoc = driverPin->instanceOwner->Location;
            std::tuple<int, int, int> driverLoc_new;
            if (inst_set.find(driverPin->instanceOwner) != inst_set.end()) {
                driverLoc_new = std::make_tuple(newLoc.first, newLoc.second, 0);
            }
            else {
                driverLoc_new = driverLoc;
            }

            std::set<std::pair<int, int>> mergedPinLocs_prev{};
            std::set<std::pair<int, int>> mergedPinLocs_new{};
            for (auto outpin : net->outpins) {
                if (!outpin->timingCritical) continue;
                std::tuple<int, int, int> sinkLoc = outpin->instanceOwner->Location;
                mergedPinLocs_prev.insert(std::make_pair(std::get<0>(sinkLoc), std::get<1>(sinkLoc)));
                if (inst_set.find(outpin->instanceOwner) != inst_set.end()) {
                    mergedPinLocs_new.insert(std::make_pair(newLoc.first, newLoc.second));
                }
                else {
                    mergedPinLocs_new.insert(std::make_pair(std::get<0>(sinkLoc), std::get<1>(sinkLoc)));
                }
            }

            for (auto loc : mergedPinLocs_prev) {
                crit_prev += std::abs(std::get<0>(loc) - std::get<0>(driverLoc)) + std::abs(std::get<1>(loc) - std::get<1>(driverLoc));
            }
            for (auto loc : mergedPinLocs_new) {
                crit_new += std::abs(std::get<0>(loc) - std::get<0>(driverLoc_new)) + std::abs(std::get<1>(loc) - std::get<1>(driverLoc_new));
            }
            result += (crit_new - crit_prev) * 2;

            // non-crit
            int noncrit_prev = 0;
            int noncrit_new = 0;
            std::set<std::pair<int, int>> rsmtPinLocs_prev{};
            std::set<std::pair<int, int>> rsmtPinLocs_new{};
            rsmtPinLocs_prev.insert(std::make_pair(std::get<0>(driverLoc), std::get<1>(driverLoc)));
            rsmtPinLocs_new.insert(std::make_pair(std::get<0>(driverLoc_new), std::get<1>(driverLoc_new)));
            for (auto outpin : net->outpins) {
                if (outpin->timingCritical) continue;
                std::tuple<int, int, int> sinkLoc = outpin->instanceOwner->Location;
                rsmtPinLocs_prev.insert(std::make_pair(std::get<0>(sinkLoc), std::get<1>(sinkLoc)));
                if (inst_set.find(outpin->instanceOwner) != inst_set.end()) {
                    rsmtPinLocs_new.insert(std::make_pair(newLoc.first, newLoc.second));
                }
                else {
                    rsmtPinLocs_new.insert(std::make_pair(std::get<0>(sinkLoc), std::get<1>(sinkLoc)));
                }
            }
            std::vector<int> xCoords_prev, yCoords_prev;
            std::vector<int> xCoords_new, yCoords_new;
            for (auto loc : rsmtPinLocs_prev) {
                xCoords_prev.push_back(loc.first);
                yCoords_prev.push_back(loc.second);
            }
            for (auto loc : rsmtPinLocs_new) {
                xCoords_new.push_back(loc.first);
                yCoords_new.push_back(loc.second);
            }
            if (xCoords_prev.size() > 1) {
                Tree mst_prev = rsmt.fltTree(xCoords_prev, yCoords_prev);
                noncrit_prev = rsmt.wirelength(mst_prev);
            }
            if (xCoords_new.size() > 1) {
                Tree mst_new = rsmt.fltTree(xCoords_new, yCoords_new);
                noncrit_new = rsmt.wirelength(mst_new);
            }
            result += noncrit_new - noncrit_prev;
        }
    }
    else {
        for (int ii = 4; ii < 8; ++ii)
        {
            for (auto InstID : tile_old_ptr->instanceMap["LUT"][ii].current_InstIDs)
            {
                assert(InstID >= 0 && InstID < int(InstArray.size()));
                assert(InstID != 0);
                inst_set.insert(InstArray[InstID]);
            }
        }
        for (int ii = 8; ii < 16; ++ii)
        {
            for (auto InstID : tile_old_ptr->instanceMap["SEQ"][ii].current_InstIDs)
            {
                assert(InstID >= 0 && InstID < int(InstArray.size()));
                assert(InstID != 0);
                inst_set.insert(InstArray[InstID]);
            }
        }
        ////////////////////////////////////
        for (int i = 0; i < (int)tile_old_ptr->netsConnected_bank1.size(); i++){
            SNet *net = NetArray[tile_old_ptr->netsConnected_bank1[i]];
            if (net->clock) continue;
            if (net->outpins.size() > 9900) continue;

            // crit
            int crit_prev = 0;
            int crit_new = 0;
            const SPin* driverPin = net->inpin;
            if (!driverPin) continue;

            std::tuple<int, int, int> driverLoc = driverPin->instanceOwner->Location;
            std::tuple<int, int, int> driverLoc_new;
            if (inst_set.find(driverPin->instanceOwner) != inst_set.end()) {
                driverLoc_new = std::make_tuple(newLoc.first, newLoc.second, 0);
            }
            else {
                driverLoc_new = driverLoc;
            }

            std::set<std::pair<int, int>> mergedPinLocs_prev{};
            std::set<std::pair<int, int>> mergedPinLocs_new{};
            for (auto outpin : net->outpins) {
                if (!outpin->timingCritical) continue;
                std::tuple<int, int, int> sinkLoc = outpin->instanceOwner->Location;
                mergedPinLocs_prev.insert(std::make_pair(std::get<0>(sinkLoc), std::get<1>(sinkLoc)));
                if (inst_set.find(outpin->instanceOwner) != inst_set.end()) {
                    mergedPinLocs_new.insert(std::make_pair(newLoc.first, newLoc.second));
                }
                else {
                    mergedPinLocs_new.insert(std::make_pair(std::get<0>(sinkLoc), std::get<1>(sinkLoc)));
                }
            }

            for (auto loc : mergedPinLocs_prev) {
                crit_prev += std::abs(std::get<0>(loc) - std::get<0>(driverLoc)) + std::abs(std::get<1>(loc) - std::get<1>(driverLoc));
            }
            for (auto loc : mergedPinLocs_new) {
                crit_new += std::abs(std::get<0>(loc) - std::get<0>(driverLoc_new)) + std::abs(std::get<1>(loc) - std::get<1>(driverLoc_new));
            }
            result += (crit_new - crit_prev) * 2;

            // non-crit
            int noncrit_prev = 0;
            int noncrit_new = 0;
            std::set<std::pair<int, int>> rsmtPinLocs_prev{};
            std::set<std::pair<int, int>> rsmtPinLocs_new{};
            rsmtPinLocs_prev.insert(std::make_pair(std::get<0>(driverLoc), std::get<1>(driverLoc)));
            rsmtPinLocs_new.insert(std::make_pair(std::get<0>(driverLoc_new), std::get<1>(driverLoc_new)));
            for (auto outpin : net->outpins) {
                if (outpin->timingCritical) continue;
                std::tuple<int, int, int> sinkLoc = outpin->instanceOwner->Location;
                rsmtPinLocs_prev.insert(std::make_pair(std::get<0>(sinkLoc), std::get<1>(sinkLoc)));
                if (inst_set.find(outpin->instanceOwner) != inst_set.end()) {
                    rsmtPinLocs_new.insert(std::make_pair(newLoc.first, newLoc.second));
                }
                else {
                    rsmtPinLocs_new.insert(std::make_pair(std::get<0>(sinkLoc), std::get<1>(sinkLoc)));
                }
            }
            std::vector<int> xCoords_prev, yCoords_prev;
            std::vector<int> xCoords_new, yCoords_new;
            for (auto loc : rsmtPinLocs_prev) {
                xCoords_prev.push_back(loc.first);
                yCoords_prev.push_back(loc.second);
            }
            for (auto loc : rsmtPinLocs_new) {
                xCoords_new.push_back(loc.first);
                yCoords_new.push_back(loc.second);
            }
            if (xCoords_prev.size() > 1) {
                Tree mst_prev = rsmt.fltTree(xCoords_prev, yCoords_prev);
                noncrit_prev = rsmt.wirelength(mst_prev);
            }
            if (xCoords_new.size() > 1) {
                Tree mst_new = rsmt.fltTree(xCoords_new, yCoords_new);
                noncrit_new = rsmt.wirelength(mst_new);
            }
            result += noncrit_new - noncrit_prev;
        }
    }
    return result; 
}