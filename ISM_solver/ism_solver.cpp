#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <sstream>
#include <string>
#include "ism_solver.h"
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
};

class ISMSolver_matching {
public:
    bool runNetworkSimplex(ISMMemory &mem, lemon::ListDigraph::Node s, lemon::ListDigraph::Node t, int supply) const; // supply是有多少个instance or space的意思
    void computeMatching(ISMMemory &mem) const;
};