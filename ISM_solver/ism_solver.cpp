#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <numeric>
#include <sstream>
#include <string>
#include "ism_solver.h"
#include "lemon/list_graph.h"
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