#include "solverI.h"
#include "solverObject.h"
#include "wirelength.h"
#include <cassert>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

std::vector<bool> dep_inst;

int pin_denMax;

int MaxIndepSetNum;

int MaxEmptyNum;

bool ISMSolver_matching_I::runNetworkSimplex(ISMMemory &mem, lemon::ListDigraph::Node s, lemon::ListDigraph::Node t, int supply) const {
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

    // int totalCost = 0;
    mem.sol.resize(mem.lNodes.size(), -1);
    for (size_t i = 0; i < mem.mArcs.size(); ++i) {
        if (ns.flow(mem.mArcs[i]) > 0) {
            const auto& p = mem.mArcPairs[i];
            mem.sol[p.first] = p.second;
            // totalCost += mem.costMtx[p.first][p.second];
        }
    }

    // mem.totalCost = totalCost;

    return true;
}

void ISMSolver_matching_I::computeMatching(ISMMemory &mem) const {
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

std::vector<size_t> ISMSolver_matching_I::realizeMatching_Instance(ISMMemory &mem, IndepSet &indepSet, const int Lib){
    indepSet.partCost.resize(indepSet.inst.size(), 0);
    indepSet.totalCost = 0;
    computeCostMatrix(mem, indepSet.inst, Lib);
    computeMatching(mem);
    for (size_t i = 0; i < indepSet.inst.size(); ++i){
        // indepSet.totalCost += mem.costMtx[i][mem.sol[i]];
        indepSet.partCost[i] = mem.costMtx[i][mem.sol[i]];
        indepSet.totalCost += mem.costMtx[i][mem.sol[i]];
    }
    return mem.sol;
}



/******************/



void ISMSolver_matching_I::buildIndependentIndepSets(std::vector<IndepSet> &set, const int maxR, const int maxIndepSetSize, const int Lib, std::vector<int> &priority){
    dep_inst.resize(45000 * 16, false);
    for (int i = 0; i < 45000 * 16; i++){
        dep_inst[i] = false;
    }
    // 编码方式SEQ：（y * 150 + x）* 16 + z，z from 0 to 15
    // 编码方式LUT：（y * 150 + x）* 8 * 2 + z * 2（+0 or +1），z from 0 to 7
    int set_cnt = 0;
    if (isLUT(Lib)){
        for (auto instId : priority){
            int inst_x = std::get<0>(InstArray[instId]->Location);
            int inst_y = std::get<1>(InstArray[instId]->Location);
            int inst_z = std::get<2>(InstArray[instId]->Location);
            int index = ((inst_y * 150 + inst_x) * 8 * 2) + (inst_z * 2);
            if(!dep_inst[index]&&!dep_inst[index+1]&&!InstArray[instId]->fixed&&isLUT(InstArray[instId]->Lib)){
                IndepSet indepSet;
                addLUTToIndepSet(indepSet, index, false, Lib);
                indepSet.type = 1;
                int Spacechoose = 2;
                // std::cout<<"Start find a new indepSet"<<std::endl;
                buildIndepSet(indepSet, index, maxR, maxIndepSetSize, Lib, Spacechoose, MaxEmptyNum);
                // std::cout<<"Finish find a new indepSet"<<std::endl;
                set.push_back(indepSet);
                set_cnt++;
            }
            if (set_cnt >= MaxIndepSetNum) break;
        }
    }
    else {
        for (auto instId : priority){
            int inst_x = std::get<0>(InstArray[instId]->Location);
            int inst_y = std::get<1>(InstArray[instId]->Location);
            int inst_z = std::get<2>(InstArray[instId]->Location);
            int index = (inst_y * 150 + inst_x) * 16 + inst_z;
            if(!dep_inst[index]&&!InstArray[instId]->fixed&&InstArray[instId]->Lib == 19){
                IndepSet indepSet;
                addSEQToIndepSet(indepSet, index, 50, 19);
                indepSet.type = 2;
                int Spacechoose = 2;
                if (index == 0) std::cout<<"Start from a new slot"<<std::endl;
                if (instId == 0) std::cout<<"Error: start from an empty instance!"<<std::endl;
                buildIndepSet(indepSet, index, maxR, maxIndepSetSize, 19, Spacechoose, MaxEmptyNum);
                set.push_back(indepSet);
                set_cnt++;
            }
            if (set_cnt >= MaxIndepSetNum) break;
        }
    }
    return;
}

void ISMSolver_matching_I::buildIndepSet(IndepSet &indepSet, const int seed, const int maxR, const int maxIndepSetSize, int Lib, int Spacechoose, int maxSpace){
    int initX = index_2_x_inst(seed);
    int initY = index_2_y_inst(seed);
    // int initZ = index_2_z_inst(seed); unused
    // use the spiral_access to get the instance in the range of maxR
    // std::size_t maxNumPoints = 2 * (maxR + 1) * (maxR) + 1; unused
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
    for (int r = maxR + 1; r < 3 * maxR; r++){
        seq.push_back(std::make_pair(initX + r, initY));
        seq.push_back(std::make_pair(initX - r, initY));
        seq.push_back(std::make_pair(initX, initY + r));
        seq.push_back(std::make_pair(initX, initY - r));
    }
    // std::cout<<"Start addInstToIndepSet"<<std::endl;
    int SetNum = 1;
    int seq_space_num = 0;
    int MaxSpaceNum = maxSpace;
    for (auto &point : seq){
        int x = point.first;
        int y = point.second;
        int index_tile = xy_2_index(x, y);
        if (index_tile < 0 || index_tile >= int(TileArray.size()) || x < 0 || x >= 150 || y < 0 || y >= 300){
            continue;
        }
        STile* tile = TileArray[index_tile];
        if (tile->type != 1) continue;
        if (isLUT(Lib)){
            // std::cout<<"Start buildLUTIndepSetPerTile"<<std::endl;
            buildLUTIndepSetPerTile(indepSet, tile, Spacechoose, index_tile, SetNum);
            // std::cout<<"Finish buildLUTIndepSetPerTile"<<std::endl;
        }
        else if (Lib == 19){
            bool hasSEQinTile = false;
            if(tile->instanceMap["SEQ"].empty()){
                hasSEQinTile = false;
            }
            else{
                for (int i = 0; i < 16; i++){
                    if (!tile->instanceMap["SEQ"][i].current_InstIDs.empty()){
                        hasSEQinTile = true;
                        break;
                    }
                }
            }
            if(hasSEQinTile){
                buildSEQIndepSetPerTile(indepSet, tile, Spacechoose, index_tile, SetNum, MaxSpaceNum, seq_space_num);
            }
        }
        // 为了方便起见，没有严格在内部定义说到50就不行了
        if (SetNum >= maxIndepSetSize){
            return;
        }
    }
    return;
}

void ISMSolver_matching_I::addLUTToIndepSet(IndepSet &indepSet, const int index, bool isSpace, const int Lib){
    int x = index_2_x_inst(index);
    int y = index_2_y_inst(index);
    int z = index_2_z_inst(index);
    indepSet.inst.push_back(index);
    // LUT和SEQ的编码方式不同，因此要分开讨论
    dep_inst[xy_2_index(x, y) * 16 + z * 2] = true;
    dep_inst[xy_2_index(x, y) * 16 + z * 2 + 1] = true;
    // STile* tile = TileArray[xy_2_index(x, y)]; unused
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

void ISMSolver_matching_I::addSEQToIndepSet(IndepSet &indepSet, const int index, int maxSpace, const int Lib){
    int x = index_2_x_inst(index);
    int y = index_2_y_inst(index);
    int z = index % 16;
    if (dep_inst[index]) return;
    indepSet.inst.push_back(index);
    // LUT和SEQ的编码方式不同，因此要分开讨论
    if (z >= 8){    //所有都必须得是true
        for (int i = 8; i < 16; i++){
            dep_inst[xy_2_index(x, y) * 16 + i] = true;
        }
    }
    else {
        for (int i = 0; i < 8; i++){
            dep_inst[xy_2_index(x, y) * 16 + i] = true;
        }
    }
    STile* tile = TileArray[xy_2_index(x, y)];
    // Update wrong!!!!!!
    // if (x == 40 && y == 125){
    //     std::cout << "mark" << std::endl;
    //     std::cout << "The structure of tile is " << std::endl;
    //     // std::cout << "The seq_choose_num_bank0 is " << std::endl;
    //     // for (int i = 0; i < 16; i++){
    //     //     std::cout << tile->seq_choose_num_bank0[i] << " ";
    //     // }
    //     // std::cout << std::endl;
    //     // std::cout << "The seq_choose_num_bank1 is " << std::endl;
    //     // for (int i = 0; i < 8; i++){
    //     //     std::cout << tile->seq_choose_num_bank1[i] << " ";
    //     // }
    //     // std::cout << std::endl;
    //     // for (int i = 0; i < 16; i++){
    //     //     std::cout << dep_inst[xy_2_index(x, y) * 16 + i] << " ";
    //     // }
    //     // std::cout << std::endl;
    //     std::cout << "The instanceMap is " << std::endl;
    //     for (auto &inst : tile->instanceMap["SEQ"]){
    //         if (inst.current_InstIDs.empty()){
    //             continue;
    //         }
    //         std::cout << "the ID is: "<<*inst.current_InstIDs.begin() << " the size is: " << inst.current_InstIDs.size() << " ; ";
    //     }
    //     std::cout << std::endl;
    //     std::cout << "The current instance is " << std::endl;
    //     for (auto &inst : tile->instanceMap["SEQ"]){
    //         for (auto &instId : inst.current_InstIDs){
    //             std::cout << instId << " ";
    //         }
    //     }
    //     std::cout << std::endl;
    //     std::cout << "The CE_bank0 is " << std::endl;
    //     for (auto &inst : tile->CE_bank0){
    //         std::cout << inst << " ";
    //     }
    //     std::cout << std::endl;
    //     std::cout << "The CE_bank1 is " << std::endl;
    //     for (auto &inst : tile->CE_bank1){
    //         std::cout << inst << " ";
    //     }
    //     std::cout << std::endl;
    //     std::cout << "The RESET_bank0 is " << std::endl;
    //     for (auto &inst : tile->RESET_bank0){
    //         std::cout << inst << " ";
    //     }
    //     std::cout << std::endl;
    //     std::cout << "The RESET_bank1 is " << std::endl;
    //     for (auto &inst : tile->RESET_bank1){
    //         std::cout << inst << " ";
    //     }
    //     std::cout << std::endl;
    // }
    std::list<int> instIDs = findSlotInstIds(index, Lib);
    SInstance* Inst = fromListToInst(instIDs, index);
    if(Inst == nullptr){    //判断是不是空的
        return;
    }
    for (auto &instId : Inst->conn){
        SInstance* inst = InstArray[instId];
        if (inst->Lib == 19){
            int x_conn = std::get<0>(inst->Location);
            int y_conn = std::get<1>(inst->Location);
            int z_conn = std::get<2>(inst->Location);
            dep_inst[xy_2_index(x_conn, y_conn) * 16 + z_conn] = true;
        }
    }
    return;
}

void ISMSolver_matching_I::buildLUTIndepSetPerTile(IndepSet &indepSet, STile *&tile, int Spacechoose, const int tile_id, int &SetNum){
    bool SpaceChooseEnough = false;
    int SpaceCount = 0;
    if(tile->instanceMap["LUT"].empty()){
        return;
    }
    int beg = 0, end = 8;
    if (tile->has_fixed_bank0){
        beg = 4;
        end = 8;
    }
    if (tile->has_fixed_bank1){
        beg = 0;
        end = 4;
    }
    if (tile->has_fixed_bank0 && tile->has_fixed_bank1){
        beg = 4;
        end = 4;
    }
    for (int i = beg; i < end; i++){
        for (int j = 0; j < 2; j++){
            if (!dep_inst[tile_id * 16 + i * 2 + j]){
                // std::cout<<"0"<<std::endl;
                if(tile->instanceMap["LUT"].empty()){
                    std::cout<<"error1"<<std::endl;
                }
                if(tile->instanceMap["LUT"].size() < 8){
                    std::cout<<"error2"<<std::endl;
                }
                if(tile->instanceMap["LUT"][i].current_InstIDs.size() == 0){
                    // std::cout<<"1"<<std::endl;
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
                    // std::cout<<"2"<<std::endl;
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
                    // std::cout<<"3"<<std::endl;
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

void ISMSolver_matching_I::buildSEQIndepSetPerTile(IndepSet &indepSet, STile *&tile, int Spacechoose, const int tile_id, int &SetNum, int maxSpace, int &SpaceNum){
    int min_index = -1;
    int min_value = 10000000;
    if(tile->instanceMap["SEQ"].empty()){
        return;
    }
    for (int i = 0; i < 8; i++){
        if (!dep_inst[tile_id * 16 + i]){
            if (min_value > tile->seq_choose_num_bank0[i]){
                min_value = tile->seq_choose_num_bank0[i];
                min_index = i;
            }
        }
    }
    if (min_index != -1){
        // bool all_dep = true;
        // for (int i = 0; i < 8;i++){
        //     if (!dep_inst[tile_id * 16 + i]){
        //         all_dep = false;
        //         break;
        //     }
        // }
        // if(!all_dep){
        SInstance* inst = fromListToInst(tile->instanceMap["SEQ"][min_index].current_InstIDs, tile_id * 16 + min_index);
        if (inst!=nullptr){
            if (!inst->fixed){
                addSEQToIndepSet(indepSet, tile_id * 16 + min_index, false, 19);
                tile->seq_choose_num_bank0[min_index]++;
                SetNum++;
            }
        }
        else if (maxSpace > SpaceNum){  //空的不能选太多
            addSEQToIndepSet(indepSet, tile_id * 16 + min_index, true, 19);
            tile->seq_choose_num_bank0[min_index]++;
            SetNum++;
            SpaceNum++;
        }
    }
        // }
    
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
        // bool all_dep = true;
        // for (int i = 8; i < 16;i++){
        //     if (!dep_inst[tile_id * 16 + i]){
        //         all_dep = false;
        //         break;
        //     }
        // }
        // if(!all_dep){
        // }
        SInstance* inst = fromListToInst(tile->instanceMap["SEQ"][min_index - 8].current_InstIDs, tile_id * 16 + min_index);
        if (inst!=nullptr){
            if (!inst->fixed){
                addSEQToIndepSet(indepSet, tile_id * 16 + min_index, false, 19);
                tile->seq_choose_num_bank1[min_index - 8]++;
                SetNum++;
            }
        }
        else if (maxSpace > SpaceNum){
            addSEQToIndepSet(indepSet, tile_id * 16 + min_index, true, 19);
            tile->seq_choose_num_bank1[min_index - 8]++;
            SetNum++;
            SpaceNum++;
        }
    }
    return;
}


/************************/


void ISMSolver_matching_I::computeCostMatrix(ISMMemory &mem, const std::vector<int> &set, const int Lib){
    mem.bboxSet.clear();
    mem.netIds.clear();
    mem.rangeSet.clear();

    mem.rangeSet.push_back(0);
    mem.costMtx.resize(set.size(), std::vector<int>(set.size(), std::numeric_limits<int>::max()));

    // costMtx[i][j]的意思是i移动到j所在的site时的cost
    // for (int i = 0; i < int(set.size()); i++){   //i 移动到 j时的cost
    //     int oldInst = set[i];
    //     for (int j = 0; j < int(set.size()); j++){
    //         int newInst = set[j];
    //         // mem.costMtx[i][j] = instanceHPWLdifference(oldInst, newInst, Lib);
    //         mem.costMtx[i][j] = instanceWLdifference(oldInst, newInst, Lib);
    //     }
    // }

    int max_threads = 7;
    std::vector<std::thread> threads(max_threads);
    std::atomic<int> task_index(0); // 当前任务索引（线程安全）

    auto worker = [&]() {
        while (true) {
            int i = task_index++;
            if (i >= int(set.size()) * int(set.size())) break;

            try {
                int m = i / int(set.size());
                int n = i % int(set.size());
                int oldInst = set[m];
                int newInst = set[n];
                mem.costMtx[m][n] = instanceWLdifference(oldInst, newInst, Lib);
            } catch (const std::exception &e) {
                std::cerr << "Thread exception: " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "Unknown exception in thread." << std::endl;
            }
        }
    };

    for (auto &thread : threads) {
        thread = std::thread(worker);
    }

    for (auto &thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    return;
}

int ISMSolver_matching_I::instanceWLdifference(const int old_index, const int new_index, const int Lib){
    if (old_index == new_index) return 0;
    int x = index_2_x_inst(new_index);
    int y = index_2_y_inst(new_index);
    int z;
    if (isLUT(Lib)){
        z = index_2_z_inst(new_index);
    }
    else {
        z = new_index % 16;
    }
    std::list<int> new_instIDs = findSlotInstIds(new_index, Lib);
    std::list<int> old_instIDs = findSlotInstIds(old_index, Lib);
    bool old_isSpace;
    SInstance* old_inst;
    STile* tile_new = TileArray[xy_2_index(x, y)];
    int pin_add = 0;
    if (isLUT(Lib)){
        old_isSpace = (old_instIDs.size() == 0) || (old_instIDs.size() == 1 && old_index % 2 == 1);
        if (old_isSpace) return 0;
        old_inst = old_index % 2 == 0 ? InstArray[*old_instIDs.begin()] : InstArray[*old_instIDs.rbegin()];
        pin_add = old_inst->inpins.size() + old_inst->outpins.size();
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
        // pindensity constraint
        if(tile_new->pin_density + pin_add > pin_denMax){
            return std::numeric_limits<int>::max();
        }
    }
    else if (Lib == 19){
        if (!old_instIDs.size()) return 0;
        old_inst = InstArray[*old_instIDs.begin()];
        // if (*old_instIDs.begin() == 707136){
        //     std::cout<<"This is the key debug point"<<std::endl;
        // }
        bool new_seq_bank = (new_index/8)%2 == 0 ? false : true;    //表示new_index是bank0还是bank1
        pin_add = old_inst->inpins.size() + old_inst->outpins.size();

        if(!isControlSetCondition(old_inst, tile_new, new_seq_bank)){
            return std::numeric_limits<int>::max();
        }
        // pindensity constraint
        if(tile_new->pin_density + pin_add > pin_denMax){
            return std::numeric_limits<int>::max();
        }
    }
    return calculate_WL_Increase(old_inst, std::make_tuple(x, y, z));
}

int ISMSolver_matching_I::instanceHPWLdifference(const int old_index, const int new_index, const int Lib){
    if (old_index == new_index){ // 这一步会不会忽略一些应该为无穷的情况？
        return 0;
    }
    int totalHPWL = 0;
    int x = index_2_x_inst(new_index);
    int y = index_2_y_inst(new_index);
    // if (isLUT(Lib)){
    //     int z = index_2_z_inst(new_index);
    // }
    // else {
    //     int z = new_index % 16;
    // }
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
        if (!old_instIDs.size()) return 0;
        old_inst = InstArray[*old_instIDs.begin()];
        // if (*old_instIDs.begin() == 707136){
        //     std::cout<<"This is the key debug point"<<std::endl;
        // }
        bool new_seq_bank = (new_index/8)%2 == 0 ? false : true;    //表示new_index是bank0还是bank1
        STile* tile_new = TileArray[xy_2_index(x, y)];

        if(!isControlSetCondition(old_inst, tile_new, new_seq_bank)){
            return std::numeric_limits<int>::max();
        }
    }
    // 通过上面的操作，可以保证得到old_inst
    int old_clockregion = ClockRegion_Info.getCRID(index_2_x_inst(old_index), index_2_y_inst(old_index));
    int new_clockregion = ClockRegion_Info.getCRID(x, y);
    for (int i = 0; i < int(old_inst->inpins.size()); i++){
        if (old_inst->inpins[i]->netID == -1){
            continue;
        }
        SNet *net = NetArray[old_inst->inpins[i]->netID];
        if(net->clock && ClockRegion_Info.clockNets[ClockRegion_Info.getCRID(x, y)].find(net->id) != ClockRegion_Info.clockNets[ClockRegion_Info.getCRID(x, y)].end()){
            if(old_clockregion != new_clockregion){
                if(ClockRegion_Info.clockNets[new_clockregion].size() + 1 > 28){
                    return std::numeric_limits<int>::max();
                }
            }
        }
        bool crit = false;
        if (net->outpins.size() + 1 == 2){
            if (net->inpin->timingCritical) crit = true;
        }
        int tmp = std::max(net->BBox_L - x, x - net->BBox_R);
        tmp = std::max(0, tmp);
        int tmp1 = std::max(net->BBox_D - y, y - net->BBox_U);
        tmp1 = std::max(0, tmp1);
        totalHPWL += tmp + tmp1;
        if (crit) totalHPWL += 2 * (tmp + tmp1);
    }
    for (int i = 0; i < int(old_inst->outpins.size()); i++){
        if (old_inst->outpins[i]->netID == -1){
            continue;
        }
        SNet *net = NetArray[old_inst->outpins[i]->netID];
        if(net->clock && ClockRegion_Info.clockNets[ClockRegion_Info.getCRID(x, y)].find(net->id) != ClockRegion_Info.clockNets[ClockRegion_Info.getCRID(x, y)].end()){
            if(old_clockregion != new_clockregion){
                if(ClockRegion_Info.clockNets[new_clockregion].size() + 1 > 28){
                    return std::numeric_limits<int>::max();
                }
            }
        }
        bool crit = false;
        if (net->outpins.size() + 1 == 2){
            if (net->inpin->timingCritical) crit = true;
        }
        int tmp = std::max(net->BBox_L - x, x - net->BBox_R);
        tmp = std::max(0, tmp);
        int tmp1 = std::max(net->BBox_D - y, y - net->BBox_U);
        tmp1 = std::max(0, tmp1);
        totalHPWL += tmp + tmp1;
        if (crit) totalHPWL += 2 * (tmp + tmp1);
    }
    return totalHPWL;
}



/************************/



bool ISMSolver_matching_I::isLUT(int Lib){
    if (Lib >= 9 && Lib <= 14){
        return true;
    }
    return false;
}



/************************/


// 通过index找出对应的instance的id的方法
std::list<int> ISMSolver_matching_I::findSlotInstIds(int index, const int Lib){
    int x = index_2_x_inst(index);
    int y = index_2_y_inst(index);
    int z = 0;
    if(isLUT(Lib)){
        z = index_2_z_inst(index);
    }
    else {
        z = index % 16;
    }
    // if (x == 92 && y == 294 && !isLUT(Lib)){
    //     std::cout<<"This is the key debug point"<<std::endl;
    // }
    int tile_index = xy_2_index(x, y);
    return isLUT(Lib) ? TileArray[tile_index]->instanceMap["LUT"][z].current_InstIDs : TileArray[tile_index]->instanceMap["SEQ"][z].current_InstIDs;
}


SInstance* ISMSolver_matching_I::fromListToInst(std::list<int> &instIDs, int index){
    if (instIDs.size() == 0){
        return nullptr;
    }
    SInstance* inst;
    inst = (index % 2 == 0) ? InstArray[*instIDs.begin()] : InstArray[*instIDs.rbegin()];
    return inst;
}



/************************/


// ControlSet的条件判断
bool ISMSolver_matching_I::isControlSetCondition(SInstance *old_inst, STile *new_tile, bool new_bank){
    std::set<int> old_inst_ce;
    std::set<int> old_inst_ck;
    std::set<int> old_inst_rs;
    for (int i = 0; i < int(old_inst->inpins.size()); i++){
        if (old_inst->inpins[i]->netID == -1){
            continue;
        }
        // if (old_inst->inpins[i]->netID == 5760 || old_inst->inpins[i]->netID == 5761)
        //     std::cout << "debug";
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

    for (int i = 0; i < int(old_inst->outpins.size()); i++){
        if (old_inst->outpins[i]->netID == -1){
            continue;
        }
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
                if (old_inst_rs.size() > 1){
                    std::cout<<"This is the key debug point"<<std::endl;
                }
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


