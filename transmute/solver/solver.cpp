#include "solver.h"

std::vector<bool> dep;  //全局的dep数组，用于记录instance是否被占用

void ISMSolver_matching::addInstToIndepSet(IndepSet &indepSet, int X, int Y, bool bank){
    STile* tile = TileArray[xy_2_index(X, Y)];
    if (bank == false){  //bank0
        dep[2 * xy_2_index(X, Y)] = true;
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
        for (int x = -r, y = 0; x < 0; x++, y--){
            seq.push_back(std::make_pair(initXY.first + x, initXY.second + y));
        }
        for (int x = 0, y = -r; y < 0; x--, y++){
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
        if (index < 0 || index >= (int)TileArray.size()){
            continue;
        }
        if(TileArray[index]->type != 1){
            continue;
        }
        if (!dep[index1]){
            addInstToIndepSet(indepSet, x, y, false);
        }
        if ((int)indepSet.inst.size() >= maxIndepSetSize){
            return;
        }
        if (!dep[index2]){
            addInstToIndepSet(indepSet, x, y, true);
        }
        if ((int)indepSet.inst.size() >= maxIndepSetSize){
            break;
        }
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
        if (dep[2 * xy_2_index(inst->X, inst->Y) + 1]){
            if (inst->pin_in_nets_bank0.size() != 0) {
                IndepSet indepSet;
                buildIndepSet(indepSet, *inst, maxR, maxIndepSetSize);
                set.push_back(indepSet);
            }
        }
    }
    return;
}