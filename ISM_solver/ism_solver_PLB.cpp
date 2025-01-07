#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <numeric>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <lemon/list_graph.h>
#include <lemon/network_simplex.h>
#include "../transmute/solver/solverObject.h"
#include "ism_solver_PLB.h"

SClockRegion clockRegion;


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

/// @inst: 需要额外再建一个conn数组，存储和其相连的instance的id

// 以bank为最小单位

void ISMSolver_matching::addInstToIndepSet(IndepSet &indepSet, int X, int Y, bool bank){
    STile* tile = TileArray[xy_2_index(X, Y)];
    if (bank == false){  //bank0
        dep[2 * xy_2_index(X, Y)] = true;
        indepSet.inst.push_back(2 * xy_2_index(X, Y));
        for (auto &pinArr : tile->pin_in_nets_bank0){   //这里只遍历了bank0中的instance
            for (int i = 0; i < pinArr.size(); i++){
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
        indepSet.inst.push_back(2 * xy_2_index(X, Y) + 1);
        for (auto &pinArr : tile->pin_in_nets_bank1){   //这里只遍历了bank0中的instance
            for (int i = 0; i < pinArr.size(); i++){
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
    std::size_t maxNumPoints = 2 * (maxR + 1) * (maxR) + 1;
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
    if (!dep[2 * xy_2_index(initXY.first, initXY.second)]){
        addInstToIndepSet(indepSet, initXY.first, initXY.second, false);
    }
    if (!dep[2 * xy_2_index(initXY.first, initXY.second) + 1]){
        addInstToIndepSet(indepSet, initXY.first, initXY.second, true);
    }
    for (auto &point : seq){
        int x = point.first;
        int y = point.second;
        int index = xy_2_index(x, y);
        int index1 = 2 * xy_2_index(x, y);
        int index2 = 2 * xy_2_index(x, y) + 1;
        if (index < 0 || index >= TileArray.size()){
            continue;
        }
        if(TileArray[index]->type != 1){
            continue;
        }
        if (!dep[index1]){
            addInstToIndepSet(indepSet, x, y, false);
        }
        if (indepSet.inst.size() >= maxIndepSetSize){
            return;
        }
        if (!dep[index2]){
            addInstToIndepSet(indepSet, x, y, true);
        }
        if (indepSet.inst.size() >= maxIndepSetSize){
            return;
        }
    }
    return;
}

int ISMSolver_matching::HPWL(const std::pair<int, int> &p1, const std::pair<int, int> &p2){
    return std::abs(p1.second - p2.second) + std::abs(p2.first - p1.first);
}

bool ISMSolver_matching::inBox(const int x, const int y, const int BBox_R, const int BBox_L, const int BBox_U, const int BBox_D){
    return x >= BBox_L && x <= BBox_R && y >= BBox_D && y <= BBox_U;
}

bool ISMSolver_matching::checkPinInTile(STile* &tile, SPin* &thisPin, bool bank){
    if (bank == false){
        for (auto &pinArr : tile->pin_in_nets_bank0){
            for (int i = 0; i < pinArr.size(); i++){
                SPin* pin = PinArray[pinArr[i]];
                if (pin->pinID == thisPin->pinID){
                    return true;
                }
            }
        }
    }
    else{
        for (auto &pinArr : tile->pin_in_nets_bank1){
            for (int i = 0; i < pinArr.size(); i++){
                SPin* pin = PinArray[pinArr[i]];
                if (pin->pinID == thisPin->pinID){
                    return true;
                }
            }
        }
    }
}

int ISMSolver_matching::tileHPWLdifference(STile* &tile, const std::pair<int, int> &newLoc, bool bank){
    int totalHPWL = 0;
    //找出pin上的netId，然后看这个net是不是有clock的
    //再看这个tile是不是和这个loc是在一个clock region中的，如果是就不需要考虑clock region的clock net增加量了
    //如果不是，就要考虑clock region的clock net增加量
    //如果已经超过28个了，相当于是不可以移到这个位置了，就直接返回一个很大的值，表示不相连
    int old_clockregion = clockRegion.getCRID(tile->X, tile->Y);
    int new_clockregion = clockRegion.getCRID(newLoc.first, newLoc.second);
    int x = newLoc.first;
    int y = newLoc.second;
    if (bank == false){
        for (int i = 0 ; i < tile->netsConnected_bank0.size(); i++){
            SNet *net = NetArray[tile->netsConnected_bank0[i]];
            if(net->clock && clockRegion.clockNets[new_clockregion].find(net->id) != clockRegion.clockNets[new_clockregion].end()){
                if(old_clockregion != new_clockregion){
                    if(clockRegion.clockNets[new_clockregion].size() + 1 > 28){
                        return std::numeric_limits<int>::max();
                    }
                }
            }
            if (inBox(x, y, net->BBox_R, net->BBox_L, net->BBox_U, net->BBox_D)){
                continue;
            }
            if ((net->outpins.size() + 1) > 16){
                if(x < net->BBox_L){
                    if(y > net->BBox_U){
                        totalHPWL += HPWL(std::make_pair(x, y), std::make_pair(net->BBox_L, net->BBox_U));
                    }
                    else if (y < net->BBox_D){
                        totalHPWL += HPWL(std::make_pair(x, y), std::make_pair(net->BBox_L, net->BBox_D));
                    }
                    else{
                        totalHPWL += std::abs(x - net->BBox_L);
                    }
                    continue;
                }
                if (x > net->BBox_R){
                    if (y > net->BBox_U){
                        totalHPWL += HPWL(std::make_pair(x, y), std::make_pair(net->BBox_R, net->BBox_U));
                    }
                    else if (y < net->BBox_D){
                        totalHPWL += HPWL(std::make_pair(x, y), std::make_pair(net->BBox_R, net->BBox_D));
                    }
                    else{
                        totalHPWL += std::abs(x - net->BBox_R);
                    }
                    continue;
                }
                if (y > net->BBox_U){
                    totalHPWL += std::abs(y - net->BBox_U);
                    continue;
                }
                if (y < net->BBox_D){
                    totalHPWL += std::abs(y - net->BBox_D);
                    continue;
                }
            }
            else{
                int newBBox_R = net->BBox_R;
                int newBBox_L = net->BBox_L;
                int newBBox_U = net->BBox_U;
                int newBBox_D = net->BBox_D;
                if (checkPinInTile(tile, net->inpin, bank)){
                    newBBox_R = std::max(newBBox_R, x);
                    newBBox_L = std::min(newBBox_L, x);
                    newBBox_U = std::max(newBBox_U, y);
                    newBBox_D = std::min(newBBox_D, y);
                }
                for (auto &pin : net->outpins){
                    if (!checkPinInTile(tile, pin, bank)){
                        newBBox_R = std::max(newBBox_R, std::get<0>(pin->instanceOwner->Location));
                        newBBox_L = std::min(newBBox_L, std::get<0>(pin->instanceOwner->Location));
                        newBBox_U = std::max(newBBox_U, std::get<1>(pin->instanceOwner->Location));
                        newBBox_D = std::min(newBBox_D, std::get<1>(pin->instanceOwner->Location));
                    }else{
                        newBBox_R = std::max(newBBox_R, x);
                        newBBox_L = std::min(newBBox_L, x);
                        newBBox_U = std::max(newBBox_U, y);
                        newBBox_D = std::min(newBBox_D, y);
                    }
                }
                totalHPWL += (HPWL(std::make_pair(newBBox_L, newBBox_D), std::make_pair(newBBox_R, newBBox_U)) 
                - HPWL(std::make_pair(net->BBox_L, net->BBox_D), std::make_pair(net->BBox_R, net->BBox_U)));
            }
        }
    }
    else{
        for (int i = 0 ; i < tile->netsConnected_bank1.size(); i++){
            SNet *net = NetArray[tile->netsConnected_bank1[i]];
            if(net->clock && clockRegion.clockNets[new_clockregion].find(net->id) != clockRegion.clockNets[new_clockregion].end()){
                if(old_clockregion != new_clockregion){
                    if(clockRegion.clockNets[new_clockregion].size() + 1 > 28){
                        return std::numeric_limits<int>::max();
                    }
                }
            }
            if (inBox(x, y, net->BBox_R, net->BBox_L, net->BBox_U, net->BBox_D)){
                continue;
            }
            if ((net->outpins.size() + 1) > 16){
                if(x < net->BBox_L){
                    if(y > net->BBox_U){
                        totalHPWL += HPWL(std::make_pair(x, y), std::make_pair(net->BBox_L, net->BBox_U));
                    }
                    else if (y < net->BBox_D){
                        totalHPWL += HPWL(std::make_pair(x, y), std::make_pair(net->BBox_L, net->BBox_D));
                    }
                    else{
                        totalHPWL += std::abs(x - net->BBox_L);
                    }
                    continue;
                }
                if (x > net->BBox_R){
                    if (y > net->BBox_U){
                        totalHPWL += HPWL(std::make_pair(x, y), std::make_pair(net->BBox_R, net->BBox_U));
                    }
                    else if (y < net->BBox_D){
                        totalHPWL += HPWL(std::make_pair(x, y), std::make_pair(net->BBox_R, net->BBox_D));
                    }
                    else{
                        totalHPWL += std::abs(x - net->BBox_R);
                    }
                    continue;
                }
                if (y > net->BBox_U){
                    totalHPWL += std::abs(y - net->BBox_U);
                    continue;
                }
                if (y < net->BBox_D){
                    totalHPWL += std::abs(y - net->BBox_D);
                    continue;
                }
            }
            else{
                int newBBox_R = net->BBox_R;
                int newBBox_L = net->BBox_L;
                int newBBox_U = net->BBox_U;
                int newBBox_D = net->BBox_D;
                if (checkPinInTile(tile, net->inpin, bank)){
                    newBBox_R = std::max(newBBox_R, x);
                    newBBox_L = std::min(newBBox_L, x);
                    newBBox_U = std::max(newBBox_U, y);
                    newBBox_D = std::min(newBBox_D, y);
                }
                for (auto &pin : net->outpins){
                    if (!checkPinInTile(tile, pin, bank)){
                        newBBox_R = std::max(newBBox_R, std::get<0>(pin->instanceOwner->Location));
                        newBBox_L = std::min(newBBox_L, std::get<0>(pin->instanceOwner->Location));
                        newBBox_U = std::max(newBBox_U, std::get<1>(pin->instanceOwner->Location));
                        newBBox_D = std::min(newBBox_D, std::get<1>(pin->instanceOwner->Location));
                    }else{
                        newBBox_R = std::max(newBBox_R, x);
                        newBBox_L = std::min(newBBox_L, x);
                        newBBox_U = std::max(newBBox_U, y);
                        newBBox_D = std::min(newBBox_D, y);
                    }
                }
                totalHPWL += (HPWL(std::make_pair(newBBox_L, newBBox_D), std::make_pair(newBBox_R, newBBox_U))
                - HPWL(std::make_pair(net->BBox_L, net->BBox_D), std::make_pair(net->BBox_R, net->BBox_U)));
            }
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
    for (int i = 0; i < set.size(); i++){   //i 移动到 j时的cost
        int oldInst = set[i];
        STile* oldTile = TileArray[oldInst/2];
        for (int j = 0; j < set.size(); j++){
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

void ISMSolver_matching::realizeMatching(ISMMemory &mem, IndepSet &indepSet){
    computeCostMatrix(mem, indepSet.inst);
    computeMatching(mem);
    int number = indepSet.inst.size();
    size_t noMatch = -1;
    for (size_t i = 0; i < mem.sol.size(); ++i){
        if(mem.sol[i] == noMatch){
            continue;
        }
        std::cout << "Instance_id:" << indepSet.inst[i/number] << " is matched to Tile_id:" << indepSet.inst[mem.sol[i]] << std::endl;
    }
    return;
}

void ISMSolver_matching::buildIndependentIndepSets(std::vector<IndepSet> &set, const int maxR, const int maxIndepSetSize){
    dep.resize(2 * TileArray.size(), false);
    for (auto &inst : TileArray){   //遍历了所有的bank
        if (inst->type != 1) continue;
        if (!dep[2 * xy_2_index(inst->X, inst->Y)]) {
            if (inst->pin_in_nets_bank0.size() != 0) {
                IndepSet indepSet;
                buildIndepSet(indepSet, *inst, maxR, maxIndepSetSize);
                set.push_back(indepSet);
            }
        }
        if (!dep[2 * xy_2_index(inst->X, inst->Y) + 1]){
            if (inst->pin_in_nets_bank1.size() != 0) {
                IndepSet indepSet;
                buildIndepSet(indepSet, *inst, maxR, maxIndepSetSize);
                set.push_back(indepSet);
            }
        }
    }
    return;
}

int main(){
    ISMMemory mem;
    ISMSolver_matching solver;
    std::vector<IndepSet> indepSets;
    solver.buildIndependentIndepSets(indepSets, 10, 50);
    for (auto &indepSet : indepSets){
        solver.realizeMatching(mem, indepSet);
    }
    return 0;
}






