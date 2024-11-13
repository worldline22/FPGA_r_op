#include "solver.h"
#include <cassert>

std::vector<bool> dep;  //全局的dep数组，用于记录instance是否被占用

void ISMSolver_matching::addInstToIndepSet(IndepSet &indepSet, int X, int Y, bool bank){
    STile* tile = TileArray[xy_2_index(X, Y)];
    if (bank == false){  //bank0
        dep[2 * xy_2_index(X, Y)] = true;
        // if (tile->netsConnected_bank0.size() == 0)
        // {
        //     if (indepSet.space_cnt >= 20) return;
        //     else indepSet.space_cnt++;
        // }
        indepSet.inst.push_back(2 * xy_2_index(X, Y));
        for (auto &pinArr : tile->pin_in_nets_bank0){   //这里只遍历了bank0中的instance
            for (int i = 0; i < (int)pinArr.size(); i++){
                SPin* pin = PinArray[pinArr[i]];
                SInstance* inst = InstArray[pin->instanceOwner->id];
                for(auto it = inst->conn.begin(); it != inst->conn.end(); ++it){
                    SInstance* inst = InstArray[*it];
                    int x = std::get<0>(inst->Location);
                    int y = std::get<1>(inst->Location);
                    if (inst->bank == 0){
                        dep[2 * xy_2_index(x, y)] = true;
                    }
                    else{
                        dep[2 * xy_2_index(x, y) + 1] = true;
                    }
                }
            }
        }
    }
    else{//bank1
        dep[2 * xy_2_index(X, Y) + 1] = true;
        // if (tile->netsConnected_bank1.size() == 0)
        // {
        //     if (indepSet.space_cnt >= 20) return;
        //     else indepSet.space_cnt++;
        // }
        indepSet.inst.push_back(2 * xy_2_index(X, Y) + 1);
        for (auto &pinArr : tile->pin_in_nets_bank1){   //这里只遍历了bank0中的instance
            for (int i = 0; i < (int)pinArr.size(); i++){
                SPin* pin = PinArray[pinArr[i]];
                SInstance* inst = InstArray[pin->instanceOwner->id];
                for(auto it = inst->conn.begin(); it != inst->conn.end(); ++it){
                    SInstance* inst = InstArray[*it];
                    int x = std::get<0>(inst->Location);
                    int y = std::get<1>(inst->Location);
                    if (inst->bank == 0){
                        dep[2 * xy_2_index(x, y)] = true;
                    }
                    else{
                        dep[2 * xy_2_index(x, y) + 1] = true;
                    }
                }
            }
        }
    }
    return;
}

void ISMSolver_matching::buildIndepSet(IndepSet &indepSet, const STile &seed, const int maxR, const int maxIndepSetSize){
    std::pair<int, int> initXY = std::make_pair(seed.X, seed.Y);
    // use the spiral_access to get the instance in the range of maxR
    // std::size_t maxNumPoints = 2 * (maxR + 1) * (maxR) + 1;
    std::vector<std::pair<int, int> > seq;
    for (int r = 1; r <= maxR; r++){
        for (int x = r, y = 0; y < r; x--, y++){
            seq.push_back(std::make_pair(initXY.first + x, initXY.second + y));
        }
        for (int x = 0, y = r; y > 0; --x, --y){
            seq.push_back(std::make_pair(initXY.first + x, initXY.second + y));
        }
        for (int x = -r, y = 0; y > -r; x++, y--){
            seq.push_back(std::make_pair(initXY.first + x, initXY.second + y));
        }
        for (int x = 0, y = -r; y < 0; x++, y++){
            seq.push_back(std::make_pair(initXY.first + x, initXY.second + y));
        }
    }
    if (!dep[2 * xy_2_index(initXY.first, initXY.second)] && !seed.has_fixed_bank0){
        addInstToIndepSet(indepSet, initXY.first, initXY.second, false);
    }
    if (!dep[2 * xy_2_index(initXY.first, initXY.second) + 1] && !seed.has_fixed_bank1){
        addInstToIndepSet(indepSet, initXY.first, initXY.second, true);
    }
    for (auto &point : seq){
        int x = point.first;
        int y = point.second;
        int index = xy_2_index(x, y);
        int index1 = 2 * xy_2_index(x, y);
        int index2 = 2 * xy_2_index(x, y) + 1;
        if (index < 0 || index >= (int)TileArray.size()){
            continue;
        }
        if(TileArray[index]->type != 1){
            continue;
        }
        if (!dep[index1] && !TileArray[index]->has_fixed_bank0){
            addInstToIndepSet(indepSet, x, y, false);
        }
        if ((int)indepSet.inst.size() >= maxIndepSetSize){
            return;
        }
        if (!dep[index2] && !TileArray[index]->has_fixed_bank1){
            addInstToIndepSet(indepSet, x, y, true);
        }
        if ((int)indepSet.inst.size() >= maxIndepSetSize){
            return;
        }
    }
    return;
}

void ISMSolver_matching::buildIndependentIndepSets(std::vector<IndepSet> &set, const int maxR, const int maxIndepSetSize, std::vector<int> &priority){
    // dep.resize(2 * TileArray.size(), false);
    // for (auto &inst : TileArray){   //遍历了所有的bank
    //     if (inst->type != 1) continue;
    //     if (!dep[2 * xy_2_index(inst->X, inst->Y)]) {
    //         if (inst->pin_in_nets_bank0.size() != 0 && !inst->has_fixed_bank0) {
    //             IndepSet indepSet;
    //             buildIndepSet(indepSet, *inst, maxR, maxIndepSetSize);
    //             set.push_back(indepSet);
    //         }
    //     }
    //     if (!dep[2 * xy_2_index(inst->X, inst->Y) + 1]){
    //         if (inst->pin_in_nets_bank1.size() != 0 && !inst->has_fixed_bank1) {
    //             IndepSet indepSet;
    //             buildIndepSet(indepSet, *inst, maxR, maxIndepSetSize);
    //             set.push_back(indepSet);
    //         }
    //     }
    // }
    dep.resize(2 * TileArray.size());
    for (int i = 0; i < 2 * (int)TileArray.size(); i++){
        dep[i] = false;
    }
    for (int inst_id : priority)
    {
        int x = std::get<0>(InstArray[inst_id]->Location);
        int y = std::get<1>(InstArray[inst_id]->Location);
        bool bank = InstArray[inst_id]->bank;
        int site = xy_2_index(x, y) * 2 + (int)bank;
        if (dep[site]) continue;
        if (TileArray[xy_2_index(x, y)]->type != 1) continue;
        if (bank == false && TileArray[xy_2_index(x, y)]->has_fixed_bank0) continue;
        if (bank == true && TileArray[xy_2_index(x, y)]->has_fixed_bank1) continue;
        IndepSet indepSet;
        indepSet.inst.clear();
        indepSet.space_cnt = 0;
        buildIndepSet(indepSet, *TileArray[xy_2_index(x, y)], maxR, maxIndepSetSize);
        set.push_back(indepSet);
    }
    return;
}


// void ISMSolver_matching::buildIndepSet(IndepSet &indepSet, const int &seed_id, const int maxR, const int maxIndepSetSize){
//     std::pair<int, int> initXY = std::make_pair(index_2_x(seed_id / 2), index_2_y(seed_id / 2));
//     initZ = seed_id % 2;
//     // use the spiral_access to get the instance in the range of maxR
//     // std::size_t maxNumPoints = 2 * (maxR + 1) * (maxR) + 1;
//     std::vector<std::pair<int, int> > seq;
//     for (int r = 1; r <= maxR; r++){
//         for (int x = r, y = 0; y < r; x--, y++){
//             seq.push_back(std::make_pair(initXY.first + x, initXY.second + y));
//         }
//         for (int x = 0, y = r; y > 0; --x, --y){
//             seq.push_back(std::make_pair(initXY.first + x, initXY.second + y));
//         }
//         for (int x = -r, y = 0; y > -r; x++, y--){
//             seq.push_back(std::make_pair(initXY.first + x, initXY.second + y));
//         }
//         for (int x = 0, y = -r; y < 0; x++, y++){
//             seq.push_back(std::make_pair(initXY.first + x, initXY.second + y));
//         }
//     }
//     if (!dep[2 * xy_2_index(initXY.first, initXY.second)]){
//         addInstToIndepSet(indepSet, initXY.first, initXY.second, false);
//     }
//     if (!dep[2 * xy_2_index(initXY.first, initXY.second) + 1]){
//         addInstToIndepSet(indepSet, initXY.first, initXY.second, true);
//     }
//     for (auto &point : seq){
//         int x = point.first;
//         int y = point.second;
//         int index = xy_2_index(x, y);
//         int index1 = 2 * xy_2_index(x, y);
//         int index2 = 2 * xy_2_index(x, y) + 1;
//         if (index < 0 || index >= (int)TileArray.size()){
//             continue;
//         }
//         if(TileArray[index]->type != 1){
//             continue;
//         }
//         if (!dep[index1]){
//             addInstToIndepSet(indepSet, x, y, false);
//         }
//         if ((int)indepSet.inst.size() >= maxIndepSetSize){
//             return;
//         }
//         if (!dep[index2]){
//             addInstToIndepSet(indepSet, x, y, true);
//         }
//         if ((int)indepSet.inst.size() >= maxIndepSetSize){
//             break;
//         }
//     }
//     return;
// }

// void ISMSolver_matching::buildIndependentIndepSets(std::vector<IndepSet> &set, const int maxR, const int maxIndepSetSize){
//     dep.resize(2 * TileArray.size(), false);
//     for (auto instP : InstArray)
//     {
//         int x = std::get<0>(instP.second->Location);
//         int y = std::get<1>(instP.second->Location);
//         int z = std::get<2>(instP.second->Location);
//         bool bank = instp.second->bank;
//         IndepSet indepSet;
//         int seed_id = xy_2_index(x, y) * 2 + (int)bank;
//         buildIndepSet(indepSet, seed_id, maxR, maxIndepSetSize);
//         set.push_back(indepSet);
//     }
//     // for (auto &inst : TileArray){   //遍历了所有的bank
//     //     if (inst->type != 1) continue;
//     //     if (!dep[2 * xy_2_index(inst->X, inst->Y)]) {
//     //         if (inst->pin_in_nets_bank0.size() != 0) {
//     //             IndepSet indepSet;
//     //             buildIndepSet(indepSet, *inst, maxR, maxIndepSetSize);
//     //             set.push_back(indepSet);
//     //         }
//     //     }
//     //     if (dep[2 * xy_2_index(inst->X, inst->Y) + 1]){
//     //         if (inst->pin_in_nets_bank1.size() != 0) {
//     //             IndepSet indepSet;
//     //             buildIndepSet(indepSet, *inst, maxR, maxIndepSetSize);
//     //             set.push_back(indepSet);
//     //         }
//     //     }
//     // }
//     return;
// }

bool ISMSolver_matching::checkPinInTile(STile* &tile, SPin* &thisPin, bool bank){
    if (bank == false){
        for (auto &pinArr : tile->pin_in_nets_bank0){
            for (int i = 0; i < (int)pinArr.size(); i++){
                SPin* pin = PinArray[pinArr[i]];
                if (pin->pinID == thisPin->pinID){
                    return true;
                }
            }
        }
    }
    else{
        for (auto &pinArr : tile->pin_in_nets_bank1){
            for (int i = 0; i < (int)pinArr.size(); i++){
                SPin* pin = PinArray[pinArr[i]];
                if (pin->pinID == thisPin->pinID){
                    return true;
                }
            }
        }
    }
    return false;
}

int ISMSolver_matching::HPWL(const std::pair<int, int> &p1, const std::pair<int, int> &p2){
    return std::abs(p1.second - p2.second) + std::abs(p2.first - p1.first);
}

bool ISMSolver_matching::inBox(const int x, const int y, const int BBox_R, const int BBox_L, const int BBox_U, const int BBox_D){
    return x >= BBox_L && x <= BBox_R && y >= BBox_D && y <= BBox_U;
}

int ISMSolver_matching::tileHPWLdifference(STile* &tile, const std::pair<int, int> &newLoc, bool bank){
    int totalHPWL = 0;
    int old_clockregion = ClockRegion_Info.getCRID(tile->X, tile->Y);
    int new_clockregion = ClockRegion_Info.getCRID(newLoc.first, newLoc.second);
    int x = newLoc.first;
    int y = newLoc.second;
    if (bank == false){
        for (int i = 0 ; i < (int)tile->netsConnected_bank0.size(); i++){
            SNet *net = NetArray[tile->netsConnected_bank0[i]];
            bool crit = false;
            if (net->outpins.size() + 1 == 2)
            {
                if (net->inpin->timingCritical) crit = true;
            }
            // if (net->outpins.size() + 1 > 16) continue; 
            if(net->clock && ClockRegion_Info.clockNets[new_clockregion].find(net->id) != ClockRegion_Info.clockNets[new_clockregion].end())
            {
                if(old_clockregion != new_clockregion)
                {
                    if(ClockRegion_Info.clockNets[new_clockregion].size() + 1 > 28)
                    {
                        return std::numeric_limits<int>::max();
                    }
                }
            }
            // if (inBox(x, y, net->BBox_R, net->BBox_L, net->BBox_U, net->BBox_D)){
            //     continue;
            // }
            // if ((net->outpins.size() + 1) > 0){
                int tmp = std::max(x - net->BBox_R, net->BBox_L - x);
                tmp = std::max(tmp, 0);
                totalHPWL += tmp;
                if (crit) totalHPWL += tmp;
                tmp = std::max(y - net->BBox_U, net->BBox_D - y);
                tmp = std::max(tmp, 0);
                totalHPWL += tmp;
                if (crit) totalHPWL += tmp;
                // if(x < net->BBox_L){
                //     if(y > net->BBox_U){
                //         totalHPWL += HPWL(std::make_pair(x, y), std::make_pair(net->BBox_L, net->BBox_U));
                //     }
                //     else if (y < net->BBox_D){
                //         totalHPWL += HPWL(std::make_pair(x, y), std::make_pair(net->BBox_L, net->BBox_D));
                //     }
                //     else{
                //         totalHPWL += std::abs(x - net->BBox_L);
                //     }
                //     continue;
                // }
                // if (x > net->BBox_R){
                //     if (y > net->BBox_U){
                //         totalHPWL += HPWL(std::make_pair(x, y), std::make_pair(net->BBox_R, net->BBox_U));
                //     }
                //     else if (y < net->BBox_D){
                //         totalHPWL += HPWL(std::make_pair(x, y), std::make_pair(net->BBox_R, net->BBox_D));
                //     }
                //     else{
                //         totalHPWL += std::abs(x - net->BBox_R);
                //     }
                //     continue;
                // }
                // if (y > net->BBox_U){
                //     totalHPWL += std::abs(y - net->BBox_U);
                //     continue;
                // }
                // if (y < net->BBox_D){
                //     totalHPWL += std::abs(y - net->BBox_D);
                //     continue;
                // }
            // }
            // else{
            //     int newBBox_R = net->BBox_R;
            //     int newBBox_L = net->BBox_L;
            //     int newBBox_U = net->BBox_U;
            //     int newBBox_D = net->BBox_D;
            //     if (checkPinInTile(tile, net->inpin, bank)){
            //         newBBox_R = std::max(newBBox_R, x);
            //         newBBox_L = std::min(newBBox_L, x);
            //         newBBox_U = std::max(newBBox_U, y);
            //         newBBox_D = std::min(newBBox_D, y);
            //     }
            //     for (auto &pin : net->outpins){
            //         if (!checkPinInTile(tile, pin, bank)){
            //             newBBox_R = std::max(newBBox_R, std::get<0>(pin->instanceOwner->Location));
            //             newBBox_L = std::min(newBBox_L, std::get<0>(pin->instanceOwner->Location));
            //             newBBox_U = std::max(newBBox_U, std::get<1>(pin->instanceOwner->Location));
            //             newBBox_D = std::min(newBBox_D, std::get<1>(pin->instanceOwner->Location));
            //         }else{
            //             newBBox_R = std::max(newBBox_R, x);
            //             newBBox_L = std::min(newBBox_L, x);
            //             newBBox_U = std::max(newBBox_U, y);
            //             newBBox_D = std::min(newBBox_D, y);
            //         }
            //     }
            //     totalHPWL += (HPWL(std::make_pair(newBBox_L, newBBox_D), std::make_pair(newBBox_R, newBBox_U)) 
            //     - HPWL(std::make_pair(net->BBox_L, net->BBox_D), std::make_pair(net->BBox_R, net->BBox_U)));
            // }
        }
    }
    else{
        for (int i = 0 ; i < (int)tile->netsConnected_bank1.size(); i++){
            SNet *net = NetArray[tile->netsConnected_bank1[i]];
            bool crit = false;
            if (net->outpins.size() + 1 == 2)
            {
                if (net->inpin->timingCritical) crit = true;
            }
            // if (net->outpins.size() + 1 > 16) continue; 
            if(net->clock && ClockRegion_Info.clockNets[new_clockregion].find(net->id) != ClockRegion_Info.clockNets[new_clockregion].end())
            {
                if(old_clockregion != new_clockregion)
                {
                    if(ClockRegion_Info.clockNets[new_clockregion].size() + 1 > 28)
                    {
                        return std::numeric_limits<int>::max();
                    }
                }
            }
            // if (inBox(x, y, net->BBox_R, net->BBox_L, net->BBox_U, net->BBox_D)){
            //     continue;
            // }
            int tmp = std::max(x - net->BBox_R, net->BBox_L - x);
                tmp = std::max(tmp, 0);
                totalHPWL += tmp;
                if (crit) totalHPWL += tmp;
                tmp = std::max(y - net->BBox_U, net->BBox_D - y);
                tmp = std::max(tmp, 0);
                totalHPWL += tmp;
                if (crit) totalHPWL += tmp;
            // if ((net->outpins.size() + 1) > 10){
            //     if(x < net->BBox_L){
            //         if(y > net->BBox_U){
            //             totalHPWL += HPWL(std::make_pair(x, y), std::make_pair(net->BBox_L, net->BBox_U));
            //         }
            //         else if (y < net->BBox_D){
            //             totalHPWL += HPWL(std::make_pair(x, y), std::make_pair(net->BBox_L, net->BBox_D));
            //         }
            //         else{
            //             totalHPWL += std::abs(x - net->BBox_L);
            //         }
            //         continue;
            //     }
            //     if (x > net->BBox_R){
            //         if (y > net->BBox_U){
            //             totalHPWL += HPWL(std::make_pair(x, y), std::make_pair(net->BBox_R, net->BBox_U));
            //         }
            //         else if (y < net->BBox_D){
            //             totalHPWL += HPWL(std::make_pair(x, y), std::make_pair(net->BBox_R, net->BBox_D));
            //         }
            //         else{
            //             totalHPWL += std::abs(x - net->BBox_R);
            //         }
            //         continue;
            //     }
            //     if (y > net->BBox_U){
            //         totalHPWL += std::abs(y - net->BBox_U);
            //         continue;
            //     }
            //     if (y < net->BBox_D){
            //         totalHPWL += std::abs(y - net->BBox_D);
            //         continue;
            //     }
            // }
            // else{
            //     int newBBox_R = net->BBox_R;
            //     int newBBox_L = net->BBox_L;
            //     int newBBox_U = net->BBox_U;
            //     int newBBox_D = net->BBox_D;
            //     if (checkPinInTile(tile, net->inpin, bank)){
            //         newBBox_R = std::max(newBBox_R, x);
            //         newBBox_L = std::min(newBBox_L, x);
            //         newBBox_U = std::max(newBBox_U, y);
            //         newBBox_D = std::min(newBBox_D, y);
            //     }
            //     for (auto &pin : net->outpins){
            //         if (!checkPinInTile(tile, pin, bank)){
            //             newBBox_R = std::max(newBBox_R, std::get<0>(pin->instanceOwner->Location));
            //             newBBox_L = std::min(newBBox_L, std::get<0>(pin->instanceOwner->Location));
            //             newBBox_U = std::max(newBBox_U, std::get<1>(pin->instanceOwner->Location));
            //             newBBox_D = std::min(newBBox_D, std::get<1>(pin->instanceOwner->Location));
            //         }else{
            //             newBBox_R = std::max(newBBox_R, x);
            //             newBBox_L = std::min(newBBox_L, x);
            //             newBBox_U = std::max(newBBox_U, y);
            //             newBBox_D = std::min(newBBox_D, y);
            //         }
            //     }
            //     totalHPWL += (HPWL(std::make_pair(newBBox_L, newBBox_D), std::make_pair(newBBox_R, newBBox_U))
            //     - HPWL(std::make_pair(net->BBox_L, net->BBox_D), std::make_pair(net->BBox_R, net->BBox_U)));
            // }
        }
    }
    return totalHPWL;
}

void ISMSolver_matching::computeCostMatrix(ISMMemory &mem, const std::vector<int> &set){
    mem.bboxSet.clear();
    mem.netIds.clear();
    mem.rangeSet.clear();

    mem.rangeSet.push_back(0);
    mem.costMtx.resize(set.size(), std::vector<int>(set.size(), std::numeric_limits<int>::max()));

    // costMtx[i][j]的意思是i移动到j所在的site时的cost
    for (int i = 0; i < (int)set.size(); i++){   //i 移动到 j时的cost
        int oldInst = set[i];
        STile* oldTile = TileArray[oldInst/2];
        for (int j = 0; j < (int)set.size(); j++){
            int newInst = set[j];
            STile* newTile = TileArray[newInst/2];
            if(oldInst % 2 == 0){
                mem.costMtx[i][j] = tileHPWLdifference(oldTile, std::make_pair(newTile->X, newTile->Y), false);
            }
            else {
                mem.costMtx[i][j] = tileHPWLdifference(oldTile, std::make_pair(newTile->X, newTile->Y), true);
            }
            
        }
    }
    return;
}

bool ISMSolver_matching::runNetworkSimplex(ISMMemory &mem, lemon::ListDigraph::Node s, lemon::ListDigraph::Node t, int supply) const {
    using Graph = lemon::ListDigraph;
    using NS = lemon::NetworkSimplex<Graph>;

    Graph& graph = mem.graph;

    // 创建容量和成本映射
    Graph::ArcMap<int> capLo(graph), capHi(graph), costMap(graph);
    for (const auto& arc : mem.mArcs) {
        capLo[arc] = 0;
        capHi[arc] = 1;
        costMap[arc] = mem.costMtx[mem.mArcPairs[&arc - &mem.mArcs[0]].first][mem.mArcPairs[&arc - &mem.mArcs[0]].second];
    }

    for (const auto& arc : mem.lArcs) {
        capLo[arc] = 0;
        capHi[arc] = 1;
        costMap[arc] = 0;
    }

    for (const auto& arc : mem.rArcs) {
        capLo[arc] = 0;
        capHi[arc] = 1;
        costMap[arc] = 0;
    }

    NS ns(graph);
    ns.lowerMap(capLo).upperMap(capHi).costMap(costMap).stSupply(s, t, supply);
    
    NS::ProblemType result = ns.run();

    if (result != NS::OPTIMAL) {
        return false;
    }

    mem.sol.resize(mem.lNodes.size(), -1);
    for (size_t i = 0; i < mem.mArcs.size(); ++i) {
        if (ns.flow(mem.mArcs[i]) > 0) {
            const auto& p = mem.mArcPairs[i];
            mem.sol[p.first] = p.second;
        }
    }

    return true;
}

void ISMSolver_matching::computeMatching(ISMMemory &mem) const {
    lemon::ListDigraph& graph = mem.graph;
    graph.clear();
    mem.lNodes.clear();
    mem.rNodes.clear();
    mem.lArcs.clear();
    mem.rArcs.clear();
    mem.mArcs.clear();
    mem.mArcPairs.clear();

    lemon::ListDigraph::Node s = graph.addNode();
    lemon::ListDigraph::Node t = graph.addNode();

    for (size_t i = 0; i < mem.costMtx.size(); ++i) {
        mem.lNodes.push_back(graph.addNode());
        mem.lArcs.push_back(graph.addArc(s, mem.lNodes.back()));

        mem.rNodes.push_back(graph.addNode());
        mem.rArcs.push_back(graph.addArc(mem.rNodes.back(), t));
    }

    size_t n = mem.costMtx.size();
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            if(mem.costMtx[i][j] != std::numeric_limits<int>::max()) {
                mem.mArcs.push_back(graph.addArc(mem.lNodes[i], mem.rNodes[j]));
                mem.mArcPairs.push_back(std::make_pair(i, j));
            }
        }
    }

    runNetworkSimplex(mem, s, t, n);

}

std::vector<size_t> ISMSolver_matching::realizeMatching(ISMMemory &mem, IndepSet &indepSet){
    computeCostMatrix(mem, indepSet.inst);
    computeMatching(mem);
    // int number = indepSet.inst.size();
    // size_t noMatch = -1;
    return mem.sol;
    // bool err = false;
    // for (size_t i = 0; i < mem.sol.size(); ++i){
    //     if(mem.sol[i] == noMatch){
    //         err = true;
    //     }
        // std::cout << mem.sol.size() << std::endl;
        // std::cout << "Instance_id:" << indepSet.inst[i%number] << "(" << index_2_x(indepSet.inst[i%number]/2) << " " << index_2_y(indepSet.inst[i%number]/2) << ") is matched to Tile_id:" << indepSet.inst[mem.sol[i]] << "(" << index_2_x(indepSet.inst[mem.sol[i]]/2) << " " << index_2_y(indepSet.inst[mem.sol[i]]/2) << ")" << std::endl;
    // }
    // assert (!err);

    // update the instance and tile...
    // the instanceMap, nets_Connected, and pin_in_nets will be updated
    // the location of instance will be updated
    // the bounding box of the net should be updated
    // CR maybe changed
}

void update_instance(IndepSet &ids)
{
    int size = ids.inst.size();
    assert(ids.inst.size() == ids.solution.size());
    std::vector<STile> tmpTile;
    tmpTile.resize(size);
    for (int i = 0; i < size; ++i)
    {
        tmpTile[i] = STile(*TileArray[ids.inst[i]/2]);
    }
    for (int i = 0; i < size; ++i)
    {
        int siteID_from = ids.inst[i];
        int siteID_to = ids.inst[ids.solution[i]];
        int arrIdx_from = i;
        STile* tile_to = TileArray[siteID_to/2];
        // 被更新的东西，应该是inst[mem.sol[i]]，而从tempArray中找inst[i]的信息
        
        if (siteID_to % 2 == 0)
        {
            if (siteID_from % 2 == 0)
            {
                tile_to->netsConnected_bank0 = tmpTile[arrIdx_from].netsConnected_bank0;
                tile_to->pin_in_nets_bank0 = tmpTile[arrIdx_from].pin_in_nets_bank0;
                // update the instanceMap
                // assert(tile_to->type==1);
                // assert(tmpTile[arrIdx_from].type==1);
                // std::cout << tile_to->instanceMap.size() << std::endl;
                // for (auto slotarrp : tile_to->instanceMap)
                // {
                //     std::cout << slotarrp.first << std::endl;
                // }
                // assert(tile_to->instanceMap.size()==4);
                // assert(tile_to->instanceMap["F7MUX"][0].current_InstIDs.size()==0);
                // assert(tile_to->instanceMap["F8MUX"][0].current_InstIDs.size()==0);
                for (int ii = 0; ii < 4; ++ii)
                {
                    tile_to->instanceMap["LUT"][ii].current_InstIDs = tmpTile[arrIdx_from].instanceMap["LUT"][ii].current_InstIDs;
                    for (auto InstID : tile_to->instanceMap["LUT"][ii].current_InstIDs)
                    {
                        SInstance* inst = InstArray[InstID];
                        inst->Location = std::make_tuple(tile_to->X, tile_to->Y, ii);
                        inst->numMov++;
                    }
                }
                for (int ii = 0; ii < 8; ++ii)
                {
                    tile_to->instanceMap["SEQ"][ii].current_InstIDs = tmpTile[arrIdx_from].instanceMap["SEQ"][ii].current_InstIDs;
                    for (auto InstID : tile_to->instanceMap["SEQ"][ii].current_InstIDs)
                    {
                        SInstance* inst = InstArray[InstID];
                        inst->Location = std::make_tuple(tile_to->X, tile_to->Y, ii);
                        inst->numMov++;
                    }
                }
                tile_to->instanceMap["CARRY4"][0] = tmpTile[arrIdx_from].instanceMap["CARRY4"][0];
                for (auto InstID : tile_to->instanceMap["CARRY4"][0].current_InstIDs)
                {
                    SInstance* inst = InstArray[InstID];
                    inst->Location = std::make_tuple(tile_to->X, tile_to->Y, 0);
                    inst->numMov++;
                }
                tile_to->instanceMap["DRAM"][0] = tmpTile[arrIdx_from].instanceMap["DRAM"][0];
                for (auto InstID : tile_to->instanceMap["DRAM"][0].current_InstIDs)
                {
                    SInstance* inst = InstArray[InstID];
                    inst->Location = std::make_tuple(tile_to->X, tile_to->Y, 0);
                    inst->numMov++;
                }
            }
            else
            {
                tile_to->netsConnected_bank0 = tmpTile[arrIdx_from].netsConnected_bank1;
                tile_to->pin_in_nets_bank0 = tmpTile[arrIdx_from].pin_in_nets_bank1;
                // update the instanceMap
                for (int ii = 0; ii < 4; ++ii)
                {
                    tile_to->instanceMap["LUT"][ii].current_InstIDs = tmpTile[arrIdx_from].instanceMap["LUT"][ii+4].current_InstIDs;
                    for (auto InstID : tile_to->instanceMap["LUT"][ii].current_InstIDs)
                    {
                        SInstance* inst = InstArray[InstID];
                        // std::cout << "we move" << InstID << "(" << std::get<0>(inst->Location) << " " << std::get<1>(inst->Location) << " " << std::get<2>(inst->Location) << " to " << tile_to->X << " " << tile_to->Y << " " << ii << std::endl;
                        inst->Location = std::make_tuple(tile_to->X, tile_to->Y, ii);
                        inst->numMov++;
                    }
                }
                for (int ii = 0; ii < 8; ++ii)
                {
                    tile_to->instanceMap["SEQ"][ii].current_InstIDs = tmpTile[arrIdx_from].instanceMap["SEQ"][ii+8].current_InstIDs;
                    for (auto InstID : tile_to->instanceMap["SEQ"][ii].current_InstIDs)
                    {
                        SInstance* inst = InstArray[InstID];
                        inst->Location = std::make_tuple(tile_to->X, tile_to->Y, ii);
                        inst->numMov++;
                    }
                }
                tile_to->instanceMap["CARRY4"][0] = tmpTile[arrIdx_from].instanceMap["CARRY4"][1];
                for (auto InstID : tile_to->instanceMap["CARRY4"][0].current_InstIDs)
                {
                    SInstance* inst = InstArray[InstID];
                    inst->Location = std::make_tuple(tile_to->X, tile_to->Y, 0);
                    inst->numMov++;
                }
                tile_to->instanceMap["DRAM"][0] = tmpTile[arrIdx_from].instanceMap["DRAM"][1];
                for (auto InstID : tile_to->instanceMap["DRAM"][0].current_InstIDs)
                {
                    SInstance* inst = InstArray[InstID];
                    inst->Location = std::make_tuple(tile_to->X, tile_to->Y, 0);
                    inst->numMov++;
                }
            }
        }
        else
        {
            if (siteID_from % 2 == 0)
            {
                tile_to->netsConnected_bank1 = tmpTile[arrIdx_from].netsConnected_bank0;
                tile_to->pin_in_nets_bank1 = tmpTile[arrIdx_from].pin_in_nets_bank0;
                // update the instanceMap
                for (int ii = 0; ii < 4; ++ii)
                {
                    tile_to->instanceMap["LUT"][ii+4].current_InstIDs = tmpTile[arrIdx_from].instanceMap["LUT"][ii].current_InstIDs;
                    for (auto InstID : tile_to->instanceMap["LUT"][ii+4].current_InstIDs)
                    {
                        SInstance* inst = InstArray[InstID];
                        // std::cout << "we move" << InstID << "(" << std::get<0>(inst->Location) << " " << std::get<1>(inst->Location) << " " << std::get<2>(inst->Location) << " to " << tile_to->X << " " << tile_to->Y << " " << ii+4 << std::endl;
                        inst->Location = std::make_tuple(tile_to->X, tile_to->Y, ii+4);
                        inst->numMov++;
                    }
                }
                for (int ii = 0; ii < 8; ++ii)
                {
                    tile_to->instanceMap["SEQ"][ii+8].current_InstIDs = tmpTile[arrIdx_from].instanceMap["SEQ"][ii].current_InstIDs;
                    for (auto InstID : tile_to->instanceMap["SEQ"][ii+8].current_InstIDs)
                    {
                        SInstance* inst = InstArray[InstID];
                        inst->Location = std::make_tuple(tile_to->X, tile_to->Y, ii+8);
                        inst->numMov++;
                    }
                }
                tile_to->instanceMap["CARRY4"][1] = tmpTile[arrIdx_from].instanceMap["CARRY4"][0];
                for (auto InstID : tile_to->instanceMap["CARRY4"][1].current_InstIDs)
                {
                    SInstance* inst = InstArray[InstID];
                    inst->Location = std::make_tuple(tile_to->X, tile_to->Y, 1);
                    inst->numMov++;
                }
                tile_to->instanceMap["DRAM"][1] = tmpTile[arrIdx_from].instanceMap["DRAM"][0];
                for (auto InstID : tile_to->instanceMap["DRAM"][1].current_InstIDs)
                {
                    SInstance* inst = InstArray[InstID];
                    inst->Location = std::make_tuple(tile_to->X, tile_to->Y, 1);
                    inst->numMov++;
                }
            }
            else
            {
                tile_to->netsConnected_bank1 = tmpTile[arrIdx_from].netsConnected_bank1;
                tile_to->pin_in_nets_bank1 = tmpTile[arrIdx_from].pin_in_nets_bank1;
                // update the instanceMap
                for (int ii = 0; ii < 4; ++ii)
                {
                    tile_to->instanceMap["LUT"][ii+4].current_InstIDs = tmpTile[arrIdx_from].instanceMap["LUT"][ii+4].current_InstIDs;
                    for (auto InstID : tile_to->instanceMap["LUT"][ii+4].current_InstIDs)
                    {
                        SInstance* inst = InstArray[InstID];
                        // std::cout << "we move" << InstID << "(" << std::get<0>(inst->Location) << " " << std::get<1>(inst->Location) << " " << std::get<2>(inst->Location) << " to " << tile_to->X << " " << tile_to->Y << " " << ii+4 << std::endl;
                        inst->Location = std::make_tuple(tile_to->X, tile_to->Y, ii+4);
                        inst->numMov++;
                    }
                }
                for (int ii = 0; ii < 8; ++ii)
                {
                    tile_to->instanceMap["SEQ"][ii+8].current_InstIDs = tmpTile[arrIdx_from].instanceMap["SEQ"][ii+8].current_InstIDs;
                    for (auto InstID : tile_to->instanceMap["SEQ"][ii+8].current_InstIDs)
                    {
                        SInstance* inst = InstArray[InstID];
                        inst->Location = std::make_tuple(tile_to->X, tile_to->Y, ii+8);
                        inst->numMov++;
                    }
                }
                tile_to->instanceMap["CARRY4"][1] = tmpTile[arrIdx_from].instanceMap["CARRY4"][1];
                for (auto InstID : tile_to->instanceMap["CARRY4"][1].current_InstIDs)
                {
                    SInstance* inst = InstArray[InstID];
                    inst->Location = std::make_tuple(tile_to->X, tile_to->Y, 1);
                    inst->numMov++;
                }
                tile_to->instanceMap["DRAM"][1] = tmpTile[arrIdx_from].instanceMap["DRAM"][1];
                for (auto InstID : tile_to->instanceMap["DRAM"][1].current_InstIDs)
                {
                    SInstance* inst = InstArray[InstID];
                    inst->Location = std::make_tuple(tile_to->X, tile_to->Y, 1);
                    inst->numMov++;
                }
            }
        }
    }

    // update the bounding box of the net

    // for (auto instanceP : glbInstMap)
    // {
    //     SInstance* inst = instanceP.second;
    //     for (auto pinp : inst->inpins)
    //     {
    //         if (pinp->pinID != -1)
    //         {
    //             auto netp = NetArry[pinp->netID];

    //         }
    //     }
    // }
    // update the CR
}

int update_net()
{
    int totalHPWL = 0;
    for (auto netpair : NetArray)
    {
        auto netp = netpair.second;
        netp->BBox_L = std::numeric_limits<int>::max();
        netp->BBox_R = -1;
        netp->BBox_U = -1;
        netp->BBox_D = std::numeric_limits<int>::max();
        auto pinp = netp->inpin;
            auto inst = pinp->instanceOwner;
            int x = std::get<0>(inst->Location);
            int y = std::get<1>(inst->Location);
            netp->BBox_L = std::min(netp->BBox_L, x);
            netp->BBox_R = std::max(netp->BBox_R, x);
            netp->BBox_U = std::max(netp->BBox_U, y);
            netp->BBox_D = std::min(netp->BBox_D, y);
            if (pinp->prop == PinProp::PIN_PROP_CLOCK && netp->clock)
            {
                int x = std::get<0>(InstArray[pinp->instanceOwner->id]->Location);
                int y = std::get<1>(InstArray[pinp->instanceOwner->id]->Location);
                int CRID = ClockRegion_Info.getCRID(x, y);
                ClockRegion_Info.clockNets[CRID].insert(netp->id);
            }
        for (auto pinp : netp->outpins)
        {
            auto inst = pinp->instanceOwner;
            int x = std::get<0>(inst->Location);
            int y = std::get<1>(inst->Location);
            netp->BBox_L = std::min(netp->BBox_L, x);
            netp->BBox_R = std::max(netp->BBox_R, x);
            netp->BBox_U = std::max(netp->BBox_U, y);
            netp->BBox_D = std::min(netp->BBox_D, y);
            if (pinp->prop == PinProp::PIN_PROP_CLOCK && netp->clock)
            {
                int x = std::get<0>(InstArray[pinp->instanceOwner->id]->Location);
                int y = std::get<1>(InstArray[pinp->instanceOwner->id]->Location);
                int CRID = ClockRegion_Info.getCRID(x, y);
                ClockRegion_Info.clockNets[CRID].insert(netp->id);
            }
        }
        totalHPWL += (netp->BBox_R - netp->BBox_L) + (netp->BBox_U - netp->BBox_D);
    }
    return totalHPWL;
}

std::set<int> update_instance_I(IndepSet &ids, int type)
{

    int size = ids.inst.size();
    assert(ids.inst.size() == ids.solution.size());
    std::vector<STile> tmpTile;
    tmpTile.resize(size);
    for (int i = 0; i < size; ++i)
    {
        tmpTile[i] = STile(*TileArray[ids.inst[i]/16]);
    }
    // (150 * y + x) * 8 * 2 + z * 2
    // 先挪instance坐标，然后记录哪些tile发生了变化，然后直接重新计算这些tile的基本信息
    std::set<int> changed_tiles;
    for (int i = 0; i < size; ++i)
    {
        int siteID_from = ids.inst[i];
        int siteID_to = ids.inst[ids.solution[i]];
        if (siteID_from == siteID_to) continue;

        int tileID_from = siteID_from / 16;
        changed_tiles.insert(tileID_from);
        STile& tile_from = tmpTile[i];
        int detail_from = siteID_from % 16;
        int inst_from = -1;
        if (type == 1) // LUT
        {
            int z = detail_from / 2;
            int zz = detail_from % 2;
            std::list<int>& from_inst = tile_from.instanceMap["LUT"][z].current_InstIDs;
            if (from_inst.size()==0) {
                inst_from = -1;
            }
            else if(from_inst.size()==1) {
                if (zz == 1) inst_from = -1;
                else inst_from = *(from_inst.begin());
            }
            else if(from_inst.size()==2) {
                if (zz == 1) inst_from = *(from_inst.rbegin());
                else inst_from = *(from_inst.begin());
            }
        }
        else if (type == 2)
        {
            inst_from = *(tile_from.instanceMap["SEQ"][detail_from].current_InstIDs.begin());
        }

        int tileID_to = siteID_to / 16;
        changed_tiles.insert(tileID_to);
        STile* tile_to = TileArray[tileID_to];
        int detail_to = siteID_to % 16;

        // 无论当前位置有没有instance，都先塞过去
        if (type == 1)
        {
            if (inst_from != -1)
            {
                // std::cout << "instance " << inst_from << ":\n";
                // std::cout << "from: " << std::get<0>(InstArray[inst_from]->Location) << " " << std::get<1>(InstArray[inst_from]->Location) << " " << std::get<2>(InstArray[inst_from]->Location) << std::endl;
                InstArray[inst_from]->Location = std::make_tuple(index_2_x(tileID_to), index_2_y(tileID_to), detail_to / 2);
                InstArray[inst_from]->numMov++;
                // std::cout << "to: " << std::get<0>(InstArray[inst_from]->Location) << " " << std::get<1>(InstArray[inst_from]->Location) << " " << std::get<2>(InstArray[inst_from]->Location) << std::endl;
            }
            int z = detail_to / 2;
            int zz = detail_to % 2;
            std::list<int>& to_list = tile_to->instanceMap["LUT"][z].current_InstIDs;
            if (to_list.size() == 0)
            {
                to_list.insert(to_list.begin(), inst_from);
            }
            if (to_list.size() == 1)
            {
                if (zz == 1) to_list.insert(++to_list.begin(), inst_from);
                else *(to_list.begin()) = inst_from;
            }
            if (to_list.size() == 2)
            {
                if (zz == 1) *(to_list.rbegin()) = inst_from;
                else *(to_list.begin()) = inst_from;
            }
        }
        else if (type == 2)
        {
            if (inst_from != -1)
            {
                InstArray[inst_from]->Location = std::make_tuple(index_2_x(tileID_to), index_2_y(tileID_to), detail_to);
                InstArray[inst_from]->numMov++;
            }
            *(tile_to->instanceMap["SEQ"][detail_to].current_InstIDs.begin()) = inst_from;
        }
    }
    return changed_tiles;
}

void update_tile_I(std::set<int> changed_tiles)
{
    //全部挪完之后最后再全局更新一次
    // 关于netsConnected，这个在挪bank的时候暂时不更新
    for (int tileID : changed_tiles)
    {
        STile* tile = TileArray[tileID];
        if (tile->type != 1) continue;
        tile->netsConnected.clear();
        tile->netsConnected_bank0.clear();
        tile->netsConnected_bank1.clear();
        tile->pin_in_nets_bank0.clear();
        tile->pin_in_nets_bank1.clear();
        tile->CE_bank0.clear();
        tile->CE_bank1.clear();
        tile->RESET_bank0.clear();
        tile->RESET_bank1.clear();
        tile->CLOCK_bank0.clear();
        tile->CLOCK_bank1.clear();
        for (int i = 0; i < 4; ++i)
        {
            auto map = tile->instanceMap;
            auto vector = map["LUT"];
            auto slot = vector[i];
            auto list = slot.current_InstIDs;
            for (int instID : list) {
                if (instID == -1)
                {
                    auto it = std::find(tile->instanceMap["LUT"][i].current_InstIDs.begin(), tile->instanceMap["LUT"][i].current_InstIDs.end(), -1);
                    tile->instanceMap["LUT"][i].current_InstIDs.erase(it);
                    continue;
                }
                SInstance* inst = InstArray[instID];
                for (auto inpinp : inst->inpins)
                {
                    if (inpinp->netID == -1) continue;
                    tile->netsConnected.insert(inpinp->netID);
                    int sizen = tile->netsConnected_bank0.size();
                    int findindex = 0;
                    for (; findindex < sizen; ++findindex)
                        if (tile->netsConnected_bank0[findindex] == inpinp->netID) break;
                    if (findindex == sizen)
                    {
                        tile->netsConnected_bank0.push_back(inpinp->netID);
                        tile->pin_in_nets_bank0.push_back(std::vector<int>{inpinp->pinID});
                    }
                    else tile->pin_in_nets_bank0[findindex].push_back(inpinp->pinID);
                    if (inpinp->prop == PinProp::PIN_PROP_CLOCK){
                        if (tile->CLOCK_bank0.find(inpinp->netID) == tile->CLOCK_bank0.end()){
                            tile->CLOCK_bank0.insert(inpinp->netID);}}
                    else if (inpinp->prop == PinProp::PIN_PROP_CE){
                        if (tile->CE_bank0.find(inpinp->netID) == tile->CE_bank0.end()){
                            tile->CE_bank0.insert(inpinp->netID);}}
                    else if (inpinp->prop == PinProp::PIN_PROP_RESET){
                        if (tile->RESET_bank0.find(inpinp->netID) == tile->RESET_bank0.end()){
                            tile->RESET_bank0.insert(inpinp->netID);}}
                }
                for (auto outpinp : inst->outpins)
                {
                    if (outpinp->netID == -1) continue;
                    tile->netsConnected.insert(outpinp->netID);
                    int sizen = tile->netsConnected_bank0.size();
                    int findindex = 0;
                    for (; findindex < sizen; ++findindex)
                        if (tile->netsConnected_bank0[findindex] == outpinp->netID) break;
                    if (findindex == sizen)
                    {
                        tile->netsConnected_bank0.push_back(outpinp->netID);
                        tile->pin_in_nets_bank0.push_back(std::vector<int>{outpinp->pinID});
                    }
                    else tile->pin_in_nets_bank0[findindex].push_back(outpinp->pinID);
                    if (outpinp->prop == PinProp::PIN_PROP_CLOCK){
                        if (tile->CLOCK_bank0.find(outpinp->netID) == tile->CLOCK_bank0.end()){
                            tile->CLOCK_bank0.insert(outpinp->netID);}}
                    else if (outpinp->prop == PinProp::PIN_PROP_CE){
                        if (tile->CE_bank0.find(outpinp->netID) == tile->CE_bank0.end()){
                            tile->CE_bank0.insert(outpinp->netID);}}
                    else if (outpinp->prop == PinProp::PIN_PROP_RESET){
                        if (tile->RESET_bank0.find(outpinp->netID) == tile->RESET_bank0.end()){
                            tile->RESET_bank0.insert(outpinp->netID);}}
                }
            }
        }
        for (int i = 4; i < 8; ++i)
        {
            for (int instID : tile->instanceMap["LUT"][i].current_InstIDs) {
                if (instID == -1)
                {
                    auto it = std::find(tile->instanceMap["LUT"][i].current_InstIDs.begin(), tile->instanceMap["LUT"][i].current_InstIDs.end(), -1);
                    tile->instanceMap["LUT"][i].current_InstIDs.erase(it);
                    continue;
                }
                SInstance* inst = InstArray[instID];
                for (auto inpinp : inst->inpins)
                {
                    if (inpinp->netID == -1) continue;
                    tile->netsConnected.insert(inpinp->netID);
                    int sizen = tile->netsConnected_bank1.size();
                    int findindex = 0;
                    for (; findindex < sizen; ++findindex)
                        if (tile->netsConnected_bank1[findindex] == inpinp->netID) break;
                    if (findindex == sizen)
                    {
                        tile->netsConnected_bank1.push_back(inpinp->netID);
                        tile->pin_in_nets_bank1.push_back(std::vector<int>{inpinp->pinID});
                    }
                    else tile->pin_in_nets_bank1[findindex].push_back(inpinp->pinID);
                    if (inpinp->prop == PinProp::PIN_PROP_CLOCK){
                        if (tile->CLOCK_bank1.find(inpinp->netID) == tile->CLOCK_bank1.end()){
                            tile->CLOCK_bank1.insert(inpinp->netID);}}
                    else if (inpinp->prop == PinProp::PIN_PROP_CE){
                        if (tile->CE_bank1.find(inpinp->netID) == tile->CE_bank1.end()){
                            tile->CE_bank1.insert(inpinp->netID);}}
                    else if (inpinp->prop == PinProp::PIN_PROP_RESET){
                        if (tile->RESET_bank1.find(inpinp->netID) == tile->RESET_bank1.end()){
                            tile->RESET_bank1.insert(inpinp->netID);}}
                }
                for (auto outpinp : inst->outpins)
                {
                    if (outpinp->netID == -1) continue;
                    tile->netsConnected.insert(outpinp->netID);
                    int sizen = tile->netsConnected_bank1.size();
                    int findindex = 0;
                    for (; findindex < sizen; ++findindex)
                        if (tile->netsConnected_bank1[findindex] == outpinp->netID) break;
                    if (findindex == sizen)
                    {
                        tile->netsConnected_bank1.push_back(outpinp->netID);
                        tile->pin_in_nets_bank1.push_back(std::vector<int>{outpinp->pinID});
                    }
                    else tile->pin_in_nets_bank1[findindex].push_back(outpinp->pinID);
                    if (outpinp->prop == PinProp::PIN_PROP_CLOCK){
                        if (tile->CLOCK_bank1.find(outpinp->netID) == tile->CLOCK_bank1.end()){
                            tile->CLOCK_bank1.insert(outpinp->netID);}}
                    else if (outpinp->prop == PinProp::PIN_PROP_CE){
                        if (tile->CE_bank1.find(outpinp->netID) == tile->CE_bank1.end()){
                            tile->CE_bank1.insert(outpinp->netID);}}
                    else if (outpinp->prop == PinProp::PIN_PROP_RESET){
                        if (tile->RESET_bank1.find(outpinp->netID) == tile->RESET_bank1.end()){
                            tile->RESET_bank1.insert(outpinp->netID);}}
                }
            }
        }
        for (int i = 0; i < 8; ++i)
        {
            for (int instID : tile->instanceMap["SEQ"][i].current_InstIDs) {
                if (instID == -1)
                {
                    auto it = std::find(tile->instanceMap["SEQ"][i].current_InstIDs.begin(), tile->instanceMap["SEQ"][i].current_InstIDs.end(), -1);
                    tile->instanceMap["SEQ"][i].current_InstIDs.erase(it);
                    continue;
                }
                SInstance* inst = InstArray[instID];
                for (auto inpinp : inst->inpins)
                {
                    if (inpinp->netID == -1) continue;
                    tile->netsConnected.insert(inpinp->netID);
                    int sizen = tile->netsConnected_bank0.size();
                    int findindex = 0;
                    for (; findindex < sizen; ++findindex)
                        if (tile->netsConnected_bank0[findindex] == inpinp->netID) break;
                    if (findindex == sizen)
                    {
                        tile->netsConnected_bank0.push_back(inpinp->netID);
                        tile->pin_in_nets_bank0.push_back(std::vector<int>{inpinp->pinID});
                    }
                    else tile->pin_in_nets_bank0[findindex].push_back(inpinp->pinID);
                    if (inpinp->prop == PinProp::PIN_PROP_CLOCK){
                        if (tile->CLOCK_bank0.find(inpinp->netID) == tile->CLOCK_bank0.end()){
                            tile->CLOCK_bank0.insert(inpinp->netID);}}
                    else if (inpinp->prop == PinProp::PIN_PROP_CE){
                        if (tile->CE_bank0.find(inpinp->netID) == tile->CE_bank0.end()){
                            tile->CE_bank0.insert(inpinp->netID);}}
                    else if (inpinp->prop == PinProp::PIN_PROP_RESET){
                        if (tile->RESET_bank0.find(inpinp->netID) == tile->RESET_bank0.end()){
                            tile->RESET_bank0.insert(inpinp->netID);}}
                }
                for (auto outpinp : inst->outpins)
                {
                    if (outpinp->netID == -1) continue;
                    tile->netsConnected.insert(outpinp->netID);
                    int sizen = tile->netsConnected_bank0.size();
                    int findindex = 0;
                    for (; findindex < sizen; ++findindex)
                        if (tile->netsConnected_bank0[findindex] == outpinp->netID) break;
                    if (findindex == sizen)
                    {
                        tile->netsConnected_bank0.push_back(outpinp->netID);
                        tile->pin_in_nets_bank0.push_back(std::vector<int>{outpinp->pinID});
                    }
                    else tile->pin_in_nets_bank0[findindex].push_back(outpinp->pinID);
                    if (outpinp->prop == PinProp::PIN_PROP_CLOCK){
                        if (tile->CLOCK_bank0.find(outpinp->netID) == tile->CLOCK_bank0.end()){
                            tile->CLOCK_bank0.insert(outpinp->netID);}}
                    else if (outpinp->prop == PinProp::PIN_PROP_CE){
                        if (tile->CE_bank0.find(outpinp->netID) == tile->CE_bank0.end()){
                            tile->CE_bank0.insert(outpinp->netID);}}
                    else if (outpinp->prop == PinProp::PIN_PROP_RESET){
                        if (tile->RESET_bank0.find(outpinp->netID) == tile->RESET_bank0.end()){
                            tile->RESET_bank0.insert(outpinp->netID);}}
                }
            }
        }
        for (int i = 8; i < 16; ++i)
        {
            for (int instID : tile->instanceMap["SEQ"][i].current_InstIDs) {
                if (instID == -1)
                {
                    auto it = std::find(tile->instanceMap["SEQ"][i].current_InstIDs.begin(), tile->instanceMap["SEQ"][i].current_InstIDs.end(), -1);
                    tile->instanceMap["SEQ"][i].current_InstIDs.erase(it);
                    continue;
                }
                SInstance* inst = InstArray[instID];
                for (auto inpinp : inst->inpins)
                {
                    if (inpinp->netID == -1) continue;
                    tile->netsConnected.insert(inpinp->netID);
                    int sizen = tile->netsConnected_bank1.size();
                    int findindex = 0;
                    for (; findindex < sizen; ++findindex)
                        if (tile->netsConnected_bank1[findindex] == inpinp->netID) break;
                    if (findindex == sizen)
                    {
                        tile->netsConnected_bank1.push_back(inpinp->netID);
                        tile->pin_in_nets_bank1.push_back(std::vector<int>{inpinp->pinID});
                    }
                    else tile->pin_in_nets_bank1[findindex].push_back(inpinp->pinID);
                    if (inpinp->prop == PinProp::PIN_PROP_CLOCK){
                        if (tile->CLOCK_bank1.find(inpinp->netID) == tile->CLOCK_bank1.end()){
                            tile->CLOCK_bank1.insert(inpinp->netID);}}
                    else if (inpinp->prop == PinProp::PIN_PROP_CE){
                        if (tile->CE_bank1.find(inpinp->netID) == tile->CE_bank1.end()){
                            tile->CE_bank1.insert(inpinp->netID);}}
                    else if (inpinp->prop == PinProp::PIN_PROP_RESET){
                        if (tile->RESET_bank1.find(inpinp->netID) == tile->RESET_bank1.end()){
                            tile->RESET_bank1.insert(inpinp->netID);}}
                }
                for (auto outpinp : inst->outpins)
                {
                    if (outpinp->netID == -1) continue;
                    tile->netsConnected.insert(outpinp->netID);
                    int sizen = tile->netsConnected_bank1.size();
                    int findindex = 0;
                    for (; findindex < sizen; ++findindex)
                        if (tile->netsConnected_bank1[findindex] == outpinp->netID) break;
                    if (findindex == sizen)
                    {
                        tile->netsConnected_bank1.push_back(outpinp->netID);
                        tile->pin_in_nets_bank1.push_back(std::vector<int>{outpinp->pinID});
                    }
                    else tile->pin_in_nets_bank1[findindex].push_back(outpinp->pinID);
                    if (outpinp->prop == PinProp::PIN_PROP_CLOCK){
                        if (tile->CLOCK_bank1.find(outpinp->netID) == tile->CLOCK_bank1.end()){
                            tile->CLOCK_bank1.insert(outpinp->netID);}}
                    else if (outpinp->prop == PinProp::PIN_PROP_CE){
                        if (tile->CE_bank1.find(outpinp->netID) == tile->CE_bank1.end()){
                            tile->CE_bank1.insert(outpinp->netID);}}
                    else if (outpinp->prop == PinProp::PIN_PROP_RESET){
                        if (tile->RESET_bank1.find(outpinp->netID) == tile->RESET_bank1.end()){
                            tile->RESET_bank1.insert(outpinp->netID);}}
                }
            }
        }
    }
}

