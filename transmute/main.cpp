#include "checker_legacy/global.h"
#include "checker_legacy/object.h"
#include "checker_legacy/lib.h"
#include "checker_legacy/arch.h"
#include "checker_legacy/netlist.h"

#include "solver/solverObject.h"
#include "solver/solver.h"

int main(int, char* argv[])
{
    std::string nodeFileName = argv[1];
    std::string netFileName = argv[2];
    std::string timingFileName = argv[3];
    std::string outputFileName = argv[4]; 

    std::string libFileName = "../benchmark/fpga.lib";
    // on server: "/home/public/Arch/fpga.lib"
    std::string sclFileName = "../benchmark/fpga.scl";
    // on server: "/home/public/Arch/fpga.scl"
    std::string clkFileName = "../benchmark/fpga.clk";
    // on server: "/home/public/Arch/fpga.clk"

    readAndCreateLib(libFileName);  // from lib.cpp
    chip.readArch(sclFileName, clkFileName);
    // chip.reportArch();
    readInputNodes(nodeFileName);
    readInputNets(netFileName);
    readInputTiming(timingFileName);
    std::cout << "  Successfully read design files." << std::endl;
    // reportDesignStatistics();   // from netlist.cpp
    std::cout << "the output file name is " << outputFileName << std::endl;

    init_tiles();
    std::cout << "Successfully initialized tiles." << std::endl;
    copy_nets();
    std::cout << "Successfully copied nets." << std::endl;
    copy_instances();
    std::cout << "Successfully copied instances." << std::endl;
    connection_setup();
    std::cout << "Successfully set up connections." << std::endl;

    // solve start
    // ISMMemory mem;
    // ISMSolver_matching solver;
    // std::vector<IndepSet> indepSets;
    // solver.buildIndependentIndepSets(indepSets, 5, 10);
    // for (auto &indepSet : indepSets){
    //     solver.realizeMatching(mem, indepSet);
    // }
    

    return 0;
}