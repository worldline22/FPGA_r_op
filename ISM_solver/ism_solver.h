#include <vector>
#include <set>
#include <list>
#include <memory>
#include <algorithm>
#include <map>
#include <string>
#include <tuple>
#include "object.h"

// ISM solver parameters
struct ISMParam {
    double xWirelenWt = 1.0;        // x direction wirelength weight  
    double yWirelenWt = 1.0;        // y direction wirelength weight
    int maxNetDegree = 100;         // maximum net degree to consider
    int maxRadius = 5;              // maximum searching radius
    int siteXStep = 1;              // x step size when searching
    int siteYInterval = 8;          // y interval when searching  
    int maxIndepSetSize = 200;      // maximum independent set size
    int minBatchImprov = 0.01;      // minimum improvement to continue
    int numSitePerCtrlSet = 8;      // number of sites per control set
    double mateCredit = 100.0;      // credit for keeping two instances together
    double flowCostScale = 1000.0;  // scale factor for flow cost
    int batchSize = 50;             // batch size for stopping condition check
    int verbose = 1;                // verbose level
};

class ISMSolver {
private:

    struct ISMInstance {
        bool fixed; // 声明 fixed 成员变量
        int cellLib; // 声明 cellLib 成员变量
        int inst_ID; // 声明 instanceName 成员变量
        std::string modelName; // 声明 modelName 成员变量
        std::tuple<int, int, int> baseLocation; // location before optimization
        std::tuple<int, int, int> optimizeLocation; // location after optimization
        std::vector<Pin*> inpins;
        std::vector<Pin*> outpins;
        std::vector<Pin*> pins;
        std::vector<int> conn;
        bool ce;    //检查是否有CE
        bool cksr;  //检查是否有clock/reset
    };

    struct ISMNet {
        int id; // 声明 id 成员变量
        bool clock; // 声明 clock 成员变量    
        std::vector<Pin*> PinArray;
        int weight; // 声明 weight 成员变量
        std::vector<int> bbox; // 声明 bbox 成员变量,(x1, y1, x2, y2)


    };

    struct ISMPin {
        int id; // 声明 id 成员变量
        std::vector<int> netID; // 声明 netID 成员变量
        std::vector<int> instID; // 声明 instID 成员变量
        int xOff; // 声明 xOff 成员变量
        int yOff; // 声明 yOff 成员变量
        int type; // 声明 type 成员变量
        bool isTimeCritical; // 声明 isTimeCritical 成员变量
    };

    struct ISMsite {
        int x, y;   //位置
        int InstID; //在这个site中的instanceID
        bool controlSet;    //是不是含有control set的site
    };

    // 这里的独立集中我们将独立的和非独立的都框定进来
    // 我们先找出所有的独立集放到一个大的vector中可以吗？
    struct ISMIndepentSet {
        std::vector<int> Set;        // 在这个独立集里面的instances
        std::vector<bool> dep;       // 每个对应的instance是不是独立的，独立为1，非独立为0 
        bool finish;                // 是否完成
        std::pair<int,int> ceSet;    // CE pair待定
        int cksr = -1;               // Clock/Reset signal 待定
    };

public:
    ISMSolver(const ISMProblem& prob, int xl = 0, int yl = 0);

    void buildProblem(const ISMProblem& prob);
    void run();

    // sortNet()
    // sortPin()    
    // sortInstance()
    // sortSite()
    // removeRedundancy()

    std::vector<ISMIndepentSet> buildIndepSet();
    void computeNetBBoxes(std::vector<int>& Nets_set);  //计算每个net的bbox
    void computeCostMatrix(std::vector<int>& Instance_set); //这里用邻接矩阵的方式建了一个cost matrix，cost为每个instance放到对应site的线长优化量

private:
    int computeHPWL(const ISMNet); //计算HPWL



};