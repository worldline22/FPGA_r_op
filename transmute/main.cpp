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

    init_tiles();
    std::cout << "Successfully initialized tiles." << std::endl;
    copy_nets();
    std::cout << "Successfully copied nets." << std::endl;
    copy_instances();
    std::cout << "Successfully copied instances." << std::endl;
    connection_setup();
    std::cout << "Successfully set up connections." << std::endl;

    
    // solve start
    ISMMemory mem;
    ISMSolver_matching solver;
    std::vector<IndepSet> indepSets;
    solver.buildIndependentIndepSets(indepSets, 10, 50);
    std::cout << "Successfully built independent indepSets." << std::endl;
    // for (auto &indepSet : indepSets){
    //     solver.realizeMatching(mem, indepSet);
    // }

    // int used_bank_cnt = 0;
    // for (auto TileP : TileArray)
    // {
    //     if (TileP->type == 1)
    //     {
    //         if (TileP->netsConnected_bank0.size() > 0)
    //         {
    //             ++used_bank_cnt;
    //         }
    //         if (TileP->netsConnected_bank1.size() > 0)
    //         {
    //             ++used_bank_cnt;
    //         }
    //     }
    // }
    // std::cout << "Used bank count: " << used_bank_cnt << std::endl;
    // int i = 0;
    // for (auto &indepSet : indepSets)
    // {
    //     ++i;
    //     std::cout << "Set" << i << " (" << indepSet.inst.size() << ") : ";
    //     for (auto instID : indepSet.inst)
    //     {
    //         std::cout << instID << " ";
    //     }
    //     std::cout << std::endl;
    // }
    int setnum = 0;
    for (auto &indepSet : indepSets)
    {
        ++setnum;
        if (indepSet.inst.size() > 30)
        {
            // std::cout << "set no." << setnum << std::endl;
            solver.realizeMatching(mem, indepSet);
        }
    }
    std::cout << "Successfully realized matching." << std::endl;

    file_output(outputFileName);

    return 0;
}