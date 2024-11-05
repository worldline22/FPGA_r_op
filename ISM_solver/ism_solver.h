#pragma once

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
    // Helper struct for independent set matching
    struct IndepSet {
        std::vector<int> set;        // instances in current independent set
        std::vector<bool> dep;       // dependency flags 
        int type = -1;               // instance type
        std::pair<int,int> ceSet;    // CE pair
        int cksr = -1;               // Clock/Reset signal
    };

    // Memory for solving ISM
    struct ISMMemory {
        std::vector<std::tuple<double, double, double, double>> bboxes;  // bounding boxes
        std::vector<std::pair<double,double> > offsets;                // pin offsets
        std::vector<int> netIds;                                      // net IDs
        std::vector<int> ranges;                                      // ranges for instances
        std::vector<std::vector<double> > costMtx;                     // cost matrix
        std::vector<int> solution;                                    // matching solution
    };

    // Member variables
    ISMParam param;                        // parameters
    std::vector<Instance*> instances;      // all instances
    std::vector<Net*> nets;               // all nets
    std::vector<Pin*> pins;               // all pins
    std::vector<Tile*> tiles;             // all tiles
    std::vector<ClockRegion*> clockRegs;  // all clock regions
    
    // Working variables
    IndepSet indepSet;                    // current independent set
    std::vector<ISMMemory> threadMem;     // per-thread memory
    std::vector<int> priority;            // instance priorities
    int iteration = 0;                    // current iteration
    double curWL = 0;                     // current wirelength
    int numThreads = 1;                   // number of threads

    // Key functions
    void buildIndepSet(Instance* seed);
    void computeNetBBoxes(const std::vector<int>& set, ISMMemory& mem);
    void computeCostMatrix(const std::vector<int>& set, ISMMemory& mem); 
    void computeMatching(ISMMemory& mem);
    void realizeMatching(const std::vector<int>& set, ISMMemory& mem);
    bool checkStopCondition();
    double computeHPWL(int maxDegree = -1);
    void updateInstancePriority();
    bool isInstFeasible(Instance* inst) const;
    
public:
    // Constructor
    ISMSolver(const ISMParam& p, int threads = 1);

    // Main interfaces  
    void addInstance(Instance* inst) { instances.push_back(inst); }
    void addNet(Net* net) { nets.push_back(net); }
    void addPin(Pin* pin) { pins.push_back(pin); }
    void addTile(Tile* tile) { tiles.push_back(tile); }
    void addClockRegion(ClockRegion* cr) { clockRegs.push_back(cr); }
    
    // Run optimization
    void run();
    
    std::pair<double,double> getWirelength() const;
    std::pair<int,int> getLocation(Instance* inst) const;
};