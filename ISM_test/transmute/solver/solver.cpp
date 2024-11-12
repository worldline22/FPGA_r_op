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
#include "solverObject.h"
#include "../checker_legacy/object.h"
#include "solver.h"
#include <cassert>

std::vector<bool> dep_inst;


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

std::vector<size_t> ISMSolver_matching::realizeMatching_Instance(ISMMemory &mem, IndepSet &indepSet, const int Lib){
    computeCostMatrix(mem, indepSet.inst, Lib);
    computeMatching(mem);
    return mem.sol;
}



/******************/



void ISMSolver_matching::buildIndependentIndepSets(std::vector<IndepSet> &set, const int maxR, const int maxIndepSetSize, const int Lib, std::vector<int> &priority){
    dep_inst.resize(45000 * 16, false);
    // 编码方式SEQ：（y * 150 + x）* 16 + z，z from 0 to 15
    // 编码方式LUT：（y * 150 + x）* 8 * 2 + z * 2（+0 or +1），z from 0 to 7
    if (isLUT(Lib)){
        for (auto instId : priority){
            int inst_x = std::get<0>(InstArray[instId]->Location);
            int inst_y = std::get<1>(InstArray[instId]->Location);
            int inst_z = std::get<2>(InstArray[instId]->Location);
            int index = ((inst_y * 150 + inst_x) * 8 * 2) + (inst_z * 2);
            if(!dep_inst[index]&&!dep_inst[index+1]&&!InstArray[instId]->fixed){
                IndepSet indepSet;
                int Spacechoose = 2;
                buildIndepSet(indepSet, instId, maxR, maxIndepSetSize, Lib, Spacechoose);
                set.push_back(indepSet);
            }
        }
    }
    else {
        for (auto instId : priority){
            int inst_x = std::get<0>(InstArray[instId]->Location);
            int inst_y = std::get<1>(InstArray[instId]->Location);
            int inst_z = std::get<2>(InstArray[instId]->Location);
            int index = (inst_y * 150 + inst_x) * 16 + inst_z;
            if(!dep_inst[index]&&!InstArray[instId]->fixed){
                IndepSet indepSet;
                int Spacechoose = 0;
                buildIndepSet(indepSet, instId, maxR, maxIndepSetSize, Lib, Spacechoose);
                set.push_back(indepSet);
            }
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
    int SetNum = 1;
    for (auto &point : seq){
        int x = point.first;
        int y = point.second;
        int index_tile = xy_2_index(x, y);
        if (index_tile < 0 || index_tile >= TileArray.size()){
            continue;
        }
        STile* tile = TileArray[index_tile];
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

void ISMSolver_matching::addLUTToIndepSet(IndepSet &indepSet, const int index, bool isSpace, const int Lib){
    int x = index_2_x_inst(index);
    int y = index_2_y_inst(index);
    int z = index_2_z_inst(index);
    indepSet.inst.push_back(index);
    // LUT和SEQ的编码方式不同，因此要分开讨论
    dep_inst[xy_2_index(x, y) * 16 + z * 2] = true;
    dep_inst[xy_2_index(x, y) * 16 + z * 2 + 1] = true;
    STile* tile = TileArray[xy_2_index(x, y)];
    std::list<int> instIDs = findSlotInstIds(index, Lib);
    SInstance* Inst = fromListToInst(instIDs, index);
    if (Inst == nullptr){
        return;
    }
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
    return;
}

void ISMSolver_matching::addSEQToIndepSet(IndepSet &indepSet, const int index, bool isSpace, const int Lib){
    int x = index_2_x_inst(index);
    int y = index_2_y_inst(index);
    int z = index_2_z_inst(index);
    indepSet.inst.push_back(index);
    // LUT和SEQ的编码方式不同，因此要分开讨论
    dep_inst[xy_2_index(x, y) * 16 + z] = true;
    if(isSpace){
        return;
    }
    STile* tile = TileArray[xy_2_index(x, y)];
    std::list<int> instIDs = findSlotInstIds(index, Lib);
    SInstance* Inst = fromListToInst(instIDs, index);
    if(Inst == nullptr){    //判断是不是空的
        return;
    }
    for (auto &instId : Inst->conn){
        SInstance* inst = InstArray[instId];
        int x_conn = std::get<0>(inst->Location);
        int y_conn = std::get<1>(inst->Location);
        int z_conn = std::get<2>(inst->Location);
        dep_inst[xy_2_index(x_conn, y_conn) * 16 + z_conn] = true;
    }
    return;
}

void ISMSolver_matching::buildLUTIndepSetPerTile(IndepSet &indepSet, STile *&tile, int Spacechoose, const int tile_id, int &SetNum){
    bool SpaceChooseEnough = false;
    int SpaceCount = 0;
    for (int i = 0; i < 8; i++){
        for (int j = 0; j < 2; j++){
            if (!dep_inst[tile_id * 16 + i * 2 + j]){
                if(tile->instanceMap["LUT"][i].current_InstIDs.size() == 0){
                    if(!SpaceChooseEnough){
                        SpaceCount++;
                        if (SpaceCount == Spacechoose){
                            SpaceChooseEnough = true;
                        }
                        addLUTToIndepSet(indepSet, tile_id * 16 + i * 2 + j, true, 9);
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
                            addLUTToIndepSet(indepSet, tile_id * 16 + i * 2 + j, true, 9);
                            SetNum++;
                        }
                    }
                    else {
                        SInstance* inst = fromListToInst(tile->instanceMap["LUT"][i].current_InstIDs, tile_id * 16 + i * 2 + j);
                        if (!inst->fixed){
                            addLUTToIndepSet(indepSet, tile_id * 16 + i * 2 + j, false, inst->Lib);
                            SetNum++;
                        }
                    }
                }
                else {
                    SInstance* inst = fromListToInst(tile->instanceMap["LUT"][i].current_InstIDs, tile_id * 16 + i * 2 + j);
                    if (!inst->fixed){
                        addLUTToIndepSet(indepSet, tile_id * 16 + i * 2 + j, false, inst->Lib);
                        SetNum++;
                    }
                }  
            }
        }
    }
    return;
}

void ISMSolver_matching::buildSEQIndepSetPerTile(IndepSet &indepSet, STile *&tile, int Spacechoose, const int tile_id, int &SetNum){
    int min_index = -1;
    int min_value = 10000000;

    for (int i = 0; i < 8; i++){
        if (!dep_inst[tile_id * 16 + i]){
            if (min_value > tile->seq_choose_num_bank0[i]){
                min_value = tile->seq_choose_num_bank0[i];
                min_index = i;
            }
        }
    }
    if (min_index != -1){
        addSEQToIndepSet(indepSet, tile_id * 16 + min_index, false, 19);
        tile->seq_choose_num_bank0[min_index]++;
        SetNum++;
    }
    
    min_index = -1;
    min_value = 10000000;
    for (int i = 8; i < 16; i++){
        if (!dep_inst[tile_id * 16 + i]){
            if (min_value > tile->seq_choose_num_bank1[i - 8]){
                min_value = tile->seq_choose_num_bank1[i - 8];
                min_index = i;
            }
        }
    }
    if (min_index != -1){
        addSEQToIndepSet(indepSet, tile_id * 16 + min_index, false, 19);
        tile->seq_choose_num_bank1[min_index - 8]++;
        SetNum++;
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

        bool new_seq_bank = (new_index/8)%2 == 0 ? false : true;    //表示new_index是bank0还是bank1
        STile* tile_new = TileArray[xy_2_index(x, y)];

        if(!isControlSetCondition(old_inst, tile_new, new_seq_bank)){
            return std::numeric_limits<int>::max();
        }
    }
    // 通过上面的操作，可以保证得到old_inst
    int old_clockregion = ClockRegion_Info.getCRID(std::get<0>(old_inst->Location), std::get<1>(old_inst->Location));
    int new_clockregion = ClockRegion_Info.getCRID(x, y);
    for (int i = 0; i < old_inst->inpins.size(); i++){
        SNet *net = NetArray[old_inst->inpins[i]->netID];
        if(net->clock && ClockRegion_Info.clockNets[ClockRegion_Info.getCRID(x, y)].find(net->id) != ClockRegion_Info.clockNets[ClockRegion_Info.getCRID(x, y)].end()){
            if(old_clockregion != new_clockregion){
                if(ClockRegion_Info.clockNets[new_clockregion].size() + 1 > 28){
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
        if(net->clock && ClockRegion_Info.clockNets[ClockRegion_Info.getCRID(x, y)].find(net->id) != ClockRegion_Info.clockNets[ClockRegion_Info.getCRID(x, y)].end()){
            if(old_clockregion != new_clockregion){
                if(ClockRegion_Info.clockNets[new_clockregion].size() + 1 > 28){
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


// ControlSet的条件判断
bool ISMSolver_matching::isControlSetCondition(SInstance *&old_inst, STile *&new_tile, bool new_bank){
    std::set<int> old_inst_ce;
    std::set<int> old_inst_ck;
    std::set<int> old_inst_rs;

    for (int i = 0; i < old_inst->inpins.size(); i++){
        SNet *net = NetArray[old_inst->inpins[i]->netID];
        if(old_inst->inpins[i]->prop == PinProp::PIN_PROP_CE){
            old_inst_ce.insert(net->id);
        }
        else if(old_inst->inpins[i]->prop == PinProp::PIN_PROP_CLOCK){
            old_inst_ck.insert(net->id);
        }
        else if(old_inst->inpins[i]->prop == PinProp::PIN_PROP_RESET){
            old_inst_rs.insert(net->id);
        }
    }

    for (int i = 0; i < old_inst->outpins.size(); i++){
        SNet *net = NetArray[old_inst->outpins[i]->netID];
        if(old_inst->outpins[i]->prop == PinProp::PIN_PROP_CE){
            old_inst_ce.insert(net->id);
        }
        else if(old_inst->outpins[i]->prop == PinProp::PIN_PROP_CLOCK){
            old_inst_ck.insert(net->id);
        }
        else if(old_inst->outpins[i]->prop == PinProp::PIN_PROP_RESET){
            old_inst_rs.insert(net->id);
        }
    }

    /*------checker-------*/
    if (!new_bank){ //新的位置的bank是0
        if (new_tile->CE_bank0.size() == 2){
            if (new_tile->CE_bank0.find(*old_inst_ce.begin()) == new_tile->CE_bank0.end()){ //没找到，超过2的要求
                return false;
            }
        }
        if (new_tile->RESET_bank0.size() == 1){
            if (*new_tile->RESET_bank0.begin() != *old_inst_rs.begin()){ //没找到，超过1的要求
                return false;
            }
        }
        if (new_tile->CLOCK_bank0.size() == 1){
            if (*new_tile->CLOCK_bank0.begin() != *old_inst_ck.begin()){ //没找到，超过1的要求
                return false;
            }
        }
    }
    else{
        if (new_tile->CE_bank1.size() == 2){
            if (new_tile->CE_bank1.find(*old_inst_ce.begin()) == new_tile->CE_bank1.end()){ //没找到，超过2的要求
                return false;
            }
        }
        if (new_tile->RESET_bank1.size() == 1){
            if (*new_tile->RESET_bank1.begin() != *old_inst_rs.begin()){ //没找到，超过1的要求
                return false;
            }
        }
        if (new_tile->CLOCK_bank1.size() == 1){
            if (*new_tile->CLOCK_bank1.begin() != *old_inst_ck.begin()){ //没找到，超过1的要求
                return false;
            }
        }
    }

    /*------checker_successful-------*/
    return true;
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

