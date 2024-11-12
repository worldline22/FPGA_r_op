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
#include <lemon/list_graph.h>
#include <lemon/network_simplex.h>

struct ISMMemory {
    lemon::ListDigraph graph;
    std::vector<lemon::ListDigraph::Node> lNodes;  // 左侧节点
    std::vector<lemon::ListDigraph::Node> rNodes;  // 右侧节点
    std::vector<lemon::ListDigraph::Arc> lArcs;    // 左侧边
    std::vector<lemon::ListDigraph::Arc> rArcs;    // 右侧边
    std::vector<lemon::ListDigraph::Arc> mArcs;    // 中间边
    std::vector<std::pair<size_t, size_t> > mArcPairs;  // 匹配边的索引对
    std::vector<std::vector<int> > costMtx; // 成本矩阵
    std::vector<size_t> sol;                    // 结果存储
    std::vector<int> rangeSet;
    std::vector<std::vector<int> > bboxSet;
    std::vector<std::vector<int> > netIds;
};

extern std::vector<bool> dep;  //全局的dep数组，用于记录instance是否被占用

struct IndepSet{
    int type;
    std::vector<int> inst;
    std::vector<size_t> solution;
    int space_cnt;
    // int cksr;
    // std::vector<int> ce;
};

class ISMSolver_matching {
public:
    bool runNetworkSimplex(ISMMemory &mem, lemon::ListDigraph::Node s, lemon::ListDigraph::Node t, int supply) const; // supply是有多少个instance or space的意思
    void computeMatching(ISMMemory &mem) const;
    void buildIndepSet(IndepSet &indepSet, const STile & seed, const int maxR, const int maxIndepSetSize);
    void addInstToIndepSet(IndepSet &indepSet, int X, int Y, bool bank);
    void computeCostMatrix(ISMMemory &mem, const std::vector<int> &set);
    std::vector<size_t> realizeMatching(ISMMemory &mem, IndepSet &indepSet);
    int HPWL(const std::pair<int, int> &p1, const std::pair<int, int> &p2);
    int tileHPWLdifference(STile* &tile, const std::pair<int, int> &newLoc, bool bank);
    bool inBox(const int x, const int y, const int BBox_R, const int BBox_L, const int BBox_U, const int BBox_D);
    bool checkPinInTile(STile* &tile, SPin* &thisPin, bool bank);
    void buildIndependentIndepSets(std::vector<IndepSet> &set, const int maxR, const int maxIndepSetSize, std::vector<int> &priority);
    void addAllsameBankInstToIndepSet();
};

void update_instance(IndepSet &ids);
void update_instance_I(IndepSet &ids);
void update_tile_I();
int update_net();

