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

std::vector<bool> dep;

struct IndepSet{
    int type;
    std::vector<int> inst;
    // int cksr;
    // std::vector<int> ce;
};

class ISMSolver_matching {
public:
    bool runNetworkSimplex(ISMMemory &mem, lemon::ListDigraph::Node s, lemon::ListDigraph::Node t, int supply) const; // supply是有多少个instance or space的意思
    void computeMatching(ISMMemory &mem) const;
    void buildIndepSet(IndepSet &indepSet, const SInstance & seed, const int maxR, const int maxIndepSetSize);
    void addInstToIndepSet(IndepSet &indepSet, const SInstance & inst);
    void computeCostMatrix(ISMMemory &mem, const std::vector<int> &set);
    void realizeMatching(ISMMemory &mem, const std::vector<int> &set);
    int HPWL(const std::pair<int, int> &p1, const std::pair<int, int> &p2);
    int tileHPWLdifference(const STile &tile, const std::pair<int, int> &newLoc);
    bool inBox(const int x, const int y, const int BBox_R, const int BBox_L, const int BBox_U, const int BBox_D);
    bool checkPinInTile(const STile &tile, SPin* &thisPin);
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

/// @inst: 需要额外再建一个conn数组，存储和其相连的instance的id

void ISMSolver_matching::addInstToIndepSet(IndepSet &indepSet, const SInstance &inst){
    dep[inst.id] = true;
    for(auto &inst : inst.conn){
        dep[inst.id] = true;
    }
    indepSet.inst.push_back(inst.id);
    return;
}

void ISMSolver_matching::buildIndepSet(IndepSet &indepSet, const SInstance &seed, const int maxR, const int maxIndepSetSize){
    std::pair<int, int> initXY = std::make_pair(get<0>(seed.Location), get<1>(seed.Location));
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
        for (int x = -r, y = 0; x < 0; x++, y--){
            seq.push_back(std::make_pair(initXY.first + x, initXY.second + y));
        }
        for (int x = 0, y = -r; y < 0; x--, y++){
            seq.push_back(std::make_pair(initXY.first + x, initXY.second + y));
        }
    }
    addInstToIndepSet(indepSet, seed);
    for (auto &point : seq){
        int x = point.first;
        int y = point.second;
        int index = xy_2_index(x, y);
        if (TileArray[index]->tpye[0] == 0){
            if (!dep[index]){
                addInstToIndepSet(indepSet, InstArray[index]);
            }
            if (indepSet.inst.size() >= maxIndepSetSize){
                break;
            }
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

bool ISMSolver_matching::checkPinInTile(const STile &tile, SPin* &thisPin){
    for (auto &pinArr : tile.pin_in_nets){
        for (auto &pin : pinArr){
            if (pin->pinID == thisPin->pinID){
                return true;
            }
        }
    }
}

int ISMSolver_matching::tileHPWLdifference(const STile &tile, const std::pair<int, int> &newLoc){
    int totalHPWL = 0;
    int x = newLoc.first;
    int y = newLoc.second;
    for (int i = 0 ; i < tile.netsConnected.size(); i++){
        SNet *net = NetArray[tile.netsConnected[i]];
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
            if (checkPinInTile(tile, net->inpin)){
                newBBox_R = std::max(newBBox_R, x);
                newBBox_L = std::min(newBBox_L, x);
                newBBox_U = std::max(newBBox_U, y);
                newBBox_D = std::min(newBBox_D, y);
            }
            for (auto &pin : net->outpins){
                if (!checkPinInTile(tile, pin)){
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
        STile oldTile = TileArray[oldInst];
        for (int j = 0; j < set.size(); j++){
            int newInst = set[j];
            STile newTile = TileArray[newInst];
            mem.costMtx[i][j] = tileHPWLdifference(oldTile, std::make_pair(std::get<0>(InstArray[newInst].Location), std::get<1>(InstArray[newInst].Location)));
        }
    }
    return;
}




