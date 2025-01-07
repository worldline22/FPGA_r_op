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

// 放入的是全局的dep数组，数组的大小是45000*16，然后算SEQ和算LUT的时候都会用到，且共用的是一个dep（算另一个多时候直接清空）
extern std::vector<bool> dep_inst;  

extern SClockRegion clockRegion;

// 算独立集，其中inst是instance的id，这个id是我自行编码的，不是cz给的InstArray的id

struct IndepSet{
    int type;
    std::vector<int> inst;
};

class ISMSolver_matching {
public:
    // 用于求解匹配问题
    bool runNetworkSimplex(ISMMemory &mem, lemon::ListDigraph::Node s, lemon::ListDigraph::Node t, int supply) const; // supply是有多少个instance or space的意思
    void computeMatching(ISMMemory &mem) const;
    void realizeMatching_Instance(ISMMemory &mem, IndepSet &indepSet, const int Lib);  

    // 用于求解独立集问题
    void buildIndependentIndepSets(std::vector<IndepSet> &set, const int maxR, const int maxIndepSetSize, const int Lib, std::vector<int> &priority);
    void buildIndepSet(IndepSet &indepSet, const int seed, const int maxR, const int maxIndepSetSize, int Lib, int Spacechoose);
    void addLUTToIndepSet(IndepSet &indepSet, const int index, bool isSpace, const int Lib);
    void addSEQToIndepSet(IndepSet &indepSet, const int index, bool isSpace, const int Lib);
    void buildLUTIndepSetPerTile(IndepSet &indepSet, STile* &tile, int Spacechoose, const int tile_id, int &SetNum);
    void buildSEQIndepSetPerTile(IndepSet &indepSet, STile* &tile, int Spacechoose, const int tile_id, int &SetNum);

    // 用于计算权重矩阵
    void computeCostMatrix(ISMMemory &mem, const std::vector<int> &set, const int Lib);
    int instanceHPWLdifference(const int old_index, const int new_index, const int Lib);

    // 判断是否是LUT
    bool isLUT(int Lib);

    // 通过index找到current_InstIDs
    std::list<int> findSlotInstIds(int index, const int Lib);
    SInstance* fromListToInst(std::list<int> &instIDs, int index);

    // ControlSet的判断
    bool isControlSetCondition(SInstance* &old_inst, STile* &new_tile, bool new_bank);
};
