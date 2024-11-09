#include "checker_legacy/global.h"
#include "checker_legacy/object.h"
#include "checker_legacy/lib.h"
#include "checker_legacy/arch.h"
#include "checker_legacy/netlist.h"

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
    chip.reportArch();
    readInputNodes(nodeFileName);
    readInputNets(netFileName);
    readInputTiming(timingFileName);
    std::cout << "  Successfully read design files." << std::endl;
    reportDesignStatistics();   // from netlist.cpp
    std::cout << "the output file name is " << outputFileName << std::endl;



    return 0;
}