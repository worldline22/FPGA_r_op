#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <numeric>
#include <sstream>
#include <string>
#include <utility>
#include <tuple>
#include <vector>
#include <lemon/list_graph.h>
#include <lemon/network_simplex.h>
#include "../transmute/solver/solverObject.h"
#include "ism_solver.h"


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

void ISMSolver_matching::realizeMatching_Instance(ISMMemory &mem, IndepSet &indepSet, const int Lib){
    computeCostMatrix(mem, indepSet.inst, Lib);
    computeMatching(mem);
    int number = indepSet.inst.size();
    size_t noMatch = -1;
    for (size_t i = 0; i < mem.sol.size(); ++i){
        if(mem.sol[i] == noMatch){
            continue;
        }
        // 这里solution返回的是自定义编码下的id，因此要将其转化为坐标则可以直接使用index_2_x_inst等函数
        // 下面就是根据index找到对应的instacne
        // std::list<int> instIDs = findSlotInstIds(indepSet.inst[i%number], Lib);
        // SInstance* inst_old = fromListToInst(instIDs, indepSet.inst[i%number]);
        // std::list<int> instIDs_new = findSlotInstIds(indepSet.inst[mem.sol[i]], Lib);
        // SInstance* inst_new = fromListToInst(instIDs_new, indepSet.inst[mem.sol[i]]);
        // instance如果是一个空指针就表示是一个space
        std::cout << "Instance_id:" << indepSet.inst[i%number] << " is matched to Tile_id:" << indepSet.inst[mem.sol[i]] << std::endl;
    }
    return;
}



/******************/



void ISMSolver_matching::buildIndependentIndepSets(std::vector<IndepSet> &set, const int maxR, const int maxIndepSetSize, const int Lib){
    dep_inst.resize(45000 * 16, false);
    // 编码方式SEQ：（y * 150 + x）* 16 + z，z from 0 to 15
    // 编码方式LUT：（y * 150 + x）* 8 * 2 + z * 2（+0 or +1），z from 0 to 7
    for (int i = 0; i < 45000 * 16 ;i++){
        if(!dep_inst[i]){
            IndepSet indepSet;
            // 这是在一个tile内部最多可以选择的最多的space的个数
            int Spacechoose = 2;
            buildIndepSet(indepSet, i, maxR, maxIndepSetSize, Lib, Spacechoose);
        }
    }
    return;
}

void ISMSolver_matching::buildIndepSet(IndepSet &indepSet, const int seed, const int maxR, const int maxIndepSetSize, int Lib, int Spacechoose){
    int initX = index_2_x_inst(seed);
    int initY = index_2_y_inst(seed);
    int initZ = index_2_z_inst(seed);
    // use the spiral_access to get the instance in the range of maxR
    std::size_t maxNumPoints = 2 * (maxR + 1) * (maxR) + 1;
    std::vector<std::pair<int, int> > seq;
    seq.push_back(std::make_pair(initX, initY));
    for (int r = 1; r <= maxR; r++){
        for (int x = r, y = 0; y < r; x--, y++){
            seq.push_back(std::make_pair(initX + x, initY + y));
        }
        for (int x = 0, y = r; y > 0; --x, --y){
            seq.push_back(std::make_pair(initX + x, initY + y));
        }
        for (int x = -r, y = 0; y > -r; x++, y--){
            seq.push_back(std::make_pair(initX + x, initY + y));
        }
        for (int x = 0, y = -r; y < 0; x++, y++){
            seq.push_back(std::make_pair(initX + x, initY + y));
        }
    }
    int SetNum = 0;
    for (auto &point : seq){
        int x = point.first;
        int y = point.second;
        STile* tile = TileArray[xy_2_index(x, y)];
        if (isLUT(Lib)){
            buildLUTIndepSetPerTile(indepSet, tile, Spacechoose, xy_2_index(x, y), SetNum);
        }
        else if (Lib == 19){
            buildSEQIndepSetPerTile(indepSet, tile, Spacechoose, xy_2_index(x, y), SetNum);
        }
        // 为了方便起见，没有严格在内部定义说到50就不行了
        if (SetNum >= maxIndepSetSize){
            return;
        }
    }
    return;
}

void ISMSolver_matching::addInstToIndepSet(IndepSet &indepSet, const int index, bool isSpace, const int Lib){
    int x = index_2_x_inst(index);
    int y = index_2_y_inst(index);
    int z = index_2_z_inst(index);
    indepSet.inst.push_back(index);
    // LUT和SEQ的编码方式不同，因此要分开讨论
    if (isLUT(Lib)){
        dep_inst[xy_2_index(x, y) * 16 + z * 2] = true;
        dep_inst[xy_2_index(x, y) * 16 + z * 2 + 1] = true;
    }
    else if (Lib == 19){
        dep_inst[xy_2_index(x, y) * 16 + z] = true;
    }
    if(isSpace){
        return;
    }
    STile* tile = TileArray[xy_2_index(x, y)];
    std::list<int> instIDs = findSlotInstIds(index, Lib);
    SInstance* Inst = fromListToInst(instIDs, index);
    if (isLUT(Lib)){
        for (auto &instId : Inst->conn){
            SInstance* inst = InstArray[instId];
            if(isLUT(inst->Lib)){   //如果都是LUT，那么就要把两个位置都占用
                int x_conn = std::get<0>(inst->Location);
                int y_conn = std::get<1>(inst->Location);
                int z_conn = std::get<2>(inst->Location);
                dep_inst[xy_2_index(x_conn, y_conn) * 16 + z_conn * 2] = true;
                dep_inst[xy_2_index(x_conn, y_conn) * 16 + z_conn * 2 + 1] = true;
                //相当于两个都要编码成true
            }
        }
    }
    else {
        for (auto &instId : Inst->conn){
            SInstance* inst = InstArray[instId];
            int x_conn = std::get<0>(inst->Location);
            int y_conn = std::get<1>(inst->Location);
            int z_conn = std::get<2>(inst->Location);
            dep_inst[xy_2_index(x_conn, y_conn) * 16 + z_conn] = true;
        }
    }
    return;
}

void ISMSolver_matching::buildLUTIndepSetPerTile(IndepSet &indepSet, STile *&tile, int Spacechoose, const int tile_id, int &SetNum){
    bool SpaceChooseEnough = false;
    int SpaceCount = 0;
    for (int i = 0; i < tile->instanceMap["LUT"].size(); i++){
        for (int j = 0; j < 2; j++){
            if (!dep_inst[tile_id * 16 + i * 2 + j]){
                if(tile->instanceMap["LUT"][i].current_InstIDs.size() == 0){
                    if(!SpaceChooseEnough){
                        SpaceCount++;
                        if (SpaceCount == Spacechoose){
                            SpaceChooseEnough = true;
                        }
                        addInstToIndepSet(indepSet, tile_id * 16 + i * 2 + j, true, 9);
                        SetNum++;
                    }
                }
                else if (tile->instanceMap["LUT"][i].current_InstIDs.size() == 1){  //如果只有一个LUT默认放在偶数的位置上
                    if (j == 1){
                        if(!SpaceChooseEnough){
                            SpaceCount++;
                            if (SpaceCount == Spacechoose){
                                SpaceChooseEnough = true;
                            }
                            addInstToIndepSet(indepSet, tile_id * 16 + i * 2 + j, true, 9);
                            SetNum++;
                        }
                    }
                    else {
                        SInstance* inst = fromListToInst(tile->instanceMap["LUT"][i].current_InstIDs, tile_id * 16 + i * 2 + j);
                        addInstToIndepSet(indepSet, tile_id * 16 + i * 2 + j, false, inst->Lib);
                        SetNum++;
                    }
                }
                else {
                    SInstance* inst = fromListToInst(tile->instanceMap["LUT"][i].current_InstIDs, tile_id * 16 + i * 2 + j);
                    addInstToIndepSet(indepSet, tile_id * 16 + i * 2 + j, false, inst->Lib);
                    SetNum++;
                }  
            }
        }
    }
    return;
}

void ISMSolver_matching::buildSEQIndepSetPerTile(IndepSet &indepSet, STile *&tile, int Spacechoose, const int tile_id, int &SetNum){
    int SpaceCount = 0;
    for (int i = 0; i < tile->instanceMap["SEQ"].size(); i++){
        if (!dep_inst[tile_id * 16 + i]){
            if (tile->instanceMap["SEQ"][i].current_InstIDs.size() == 0){
                SpaceCount++;
                if (SpaceCount <= Spacechoose){
                    addInstToIndepSet(indepSet, tile_id * 16 + i, true, 19);
                    SetNum++;
                }
            }
            else{
                SInstance* inst = fromListToInst(tile->instanceMap["SEQ"][i].current_InstIDs, tile_id * 16 + i);
                addInstToIndepSet(indepSet, tile_id * 16 + i, false, 19);
                SetNum++;
            }
        }
    }
    return;
}


/************************/


void ISMSolver_matching::computeCostMatrix(ISMMemory &mem, const std::vector<int> &set, const int Lib){
    mem.bboxSet.clear();
    mem.netIds.clear();
    mem.rangeSet.clear();

    mem.rangeSet.push_back(0);
    mem.costMtx.resize(set.size(), std::vector<int>(set.size(), std::numeric_limits<int>::max()));

    // costMtx[i][j]的意思是i移动到j所在的site时的cost
    for (int i = 0; i < set.size(); i++){   //i 移动到 j时的cost
        int oldInst = set[i];
        for (int j = 0; j < set.size(); j++){
            int newInst = set[j];
            mem.costMtx[i][j] = instanceHPWLdifference(oldInst, newInst, Lib);
        }
    }
    return;
}

int ISMSolver_matching::instanceHPWLdifference(const int old_index, const int new_index, const int Lib){
    if (old_index == new_index){
        return 0;
    }
    int totalHPWL = 0;
    int x = index_2_x_inst(new_index);
    int y = index_2_y_inst(new_index);
    int z = index_2_z_inst(new_index);
    std::list<int> new_instIDs = findSlotInstIds(new_index, Lib);
    std::list<int> old_instIDs = findSlotInstIds(old_index, Lib);
    bool old_isSpace;
    SInstance* old_inst;
    if (isLUT(Lib)){
        old_isSpace = (old_instIDs.size() == 0) || (old_instIDs.size() == 1 && old_index % 2 == 1);
        if (old_isSpace) return 0;
        old_inst = old_index % 2 == 0 ? InstArray[*old_instIDs.begin()] : InstArray[*old_instIDs.rbegin()];
        if (new_instIDs.size() == 1){
            if (new_index % 2 == 1){    //表示只装了一个LUT且这个new_index是奇数，表示一个空位
                if(( old_inst -> Lib + InstArray[*new_instIDs.begin()] -> Lib) > 22){
                    return std::numeric_limits<int>::max();
                }
            }
            // 如果本身就是一个LUT，size==1表示他的另一个位置是space，这个site不存在限制条件
        }
        if (new_instIDs.size() == 2){
            // 找出另外一个LUT，看看是不是不满足条件
            int another_Lib = new_index % 2 == 0 ? InstArray[*new_instIDs.rbegin()] -> Lib : InstArray[*new_instIDs.begin()] -> Lib;    //找出另一个LUT的Lib
            if ((old_inst -> Lib + another_Lib) > 22){
                return std::numeric_limits<int>::max();
            }
        }
    }
    else if (Lib == 19){
        old_isSpace = (old_instIDs.size() == 0);
        if (old_isSpace) return 0;
        old_inst = InstArray[*old_instIDs.begin()];
    }
    // 通过上面的操作，可以保证得到old_inst
    int old_clockregion = clockRegion.getCRID(get<0>(old_inst->Location), get<1>(old_inst->Location));
    int new_clockregion = clockRegion.getCRID(x, y);
    for (int i = 0; i < old_inst->inpins.size(); i++){
        SNet *net = NetArray[old_inst->inpins[i]->netID];
        if(net->clock && clockRegion.clockNets[clockRegion.getCRID(x, y)].find(net->id) != clockRegion.clockNets[clockRegion.getCRID(x, y)].end()){
            if(old_clockregion != new_clockregion){
                if(clockRegion.clockNets[new_clockregion].size() + 1 > 28){
                    return std::numeric_limits<int>::max();
                }
            }
        }

        int tmp = std::max(net->BBox_L - x, x - net->BBox_R);
        tmp = std::max(0, tmp);
        int tmp1 = std::max(net->BBox_D - y, y - net->BBox_U);
        tmp1 = std::max(0, tmp1);
        totalHPWL += tmp + tmp1;
    }
    for (int i = 0; i < old_inst->outpins.size(); i++){
        SNet *net = NetArray[old_inst->inpins[i]->netID];
        if(net->clock && clockRegion.clockNets[clockRegion.getCRID(x, y)].find(net->id) != clockRegion.clockNets[clockRegion.getCRID(x, y)].end()){
            if(old_clockregion != new_clockregion){
                if(clockRegion.clockNets[new_clockregion].size() + 1 > 28){
                    return std::numeric_limits<int>::max();
                }
            }
        }
        int tmp = std::max(net->BBox_L - x, x - net->BBox_R);
        tmp = std::max(0, tmp);
        int tmp1 = std::max(net->BBox_D - y, y - net->BBox_U);
        tmp1 = std::max(0, tmp1);
        totalHPWL += tmp + tmp1;
    }
    return totalHPWL;
}



/************************/



bool ISMSolver_matching::isLUT(int Lib){
    if (Lib >= 9 && Lib <= 14){
        return true;
    }
    return false;
}



/************************/


// 通过index找出对应的instance的id的方法
std::list<int> ISMSolver_matching::findSlotInstIds(int index, const int Lib){
    int x = index_2_x_inst(index);
    int y = index_2_y_inst(index);
    int z = index_2_z_inst(index);
    int tile_index = xy_2_index(x, y);
    return isLUT(Lib) ? TileArray[tile_index]->instanceMap["LUT"][z].current_InstIDs : TileArray[tile_index]->instanceMap["SEQ"][z].current_InstIDs;
}


SInstance* ISMSolver_matching::fromListToInst(std::list<int> &instIDs, int index){
    if (instIDs.size() == 0){
        return nullptr;
    }
    return index % 2 == 0 ? InstArray[*instIDs.begin()] : InstArray[*instIDs.rbegin()];
}



/************************/




