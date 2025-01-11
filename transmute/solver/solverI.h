#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <numeric>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include "solverObject.h"
#include "solver.h"
#include <lemon/list_graph.h>
#include <lemon/network_simplex.h>

extern std::vector<bool> dep_inst; 

extern int pin_denMax;

extern int MaxIndepSetNum;

extern int MaxEmptyNum;

class ISMSolver_matching_I {
public:
    // 用于求解匹配问题
    bool runNetworkSimplex(ISMMemory &mem, lemon::ListDigraph::Node s, lemon::ListDigraph::Node t, int supply) const; // supply是有多少个instance or space的意思
    void computeMatching(ISMMemory &mem) const;
    std::vector<size_t> realizeMatching_Instance(ISMMemory &mem, IndepSet &indepSet, const int Lib);  

    // 用于求解独立集问题
    void buildIndependentIndepSets(std::vector<IndepSet> &set, const int maxR, const int maxIndepSetSize, const int Lib, std::vector<int> &priority);
    void buildIndepSet(IndepSet &indepSet, const int seed, const int maxR, const int maxIndepSetSize, int Lib, int Spacechoose, int maxSpace);
    void addLUTToIndepSet(IndepSet &indepSet, const int index, bool isSpace, const int Lib);
    void addSEQToIndepSet(IndepSet &indepSet, const int index, int maxSpace, const int Lib);
    void buildLUTIndepSetPerTile(IndepSet &indepSet, STile* &tile, int Spacechoose, const int tile_id, int &SetNum);
    void buildSEQIndepSetPerTile(IndepSet &indepSet, STile* &tile, int Spacechoose, const int tile_id, int &SetNum, int maxSpace, int &SpaceNum);

    // 用于计算权重矩阵
    void computeCostMatrix(ISMMemory &mem, const std::vector<int> &set, const int Lib);
    int instanceHPWLdifference(const int old_index, const int new_index, const int Lib);
    int instanceWLdifference(const int old_index, const int new_index, const int Lib);

    // 判断是否是LUT
    bool isLUT(int Lib);

    // 通过index找到current_InstIDs
    std::list<int> findSlotInstIds(int index, const int Lib);
    SInstance* fromListToInst(std::list<int> &instIDs, int index);

    // ControlSet的判断
    bool isControlSetCondition(SInstance* old_inst, STile* new_tile, bool new_bank);
};