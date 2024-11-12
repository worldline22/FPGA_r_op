#include "checker_legacy/global.h"
#include "checker_legacy/object.h"
#include "checker_legacy/lib.h"
#include "checker_legacy/arch.h"
#include "checker_legacy/netlist.h"

#include "solver/solverObject.h"
#include "solver/solver.h"
#include <cassert>
#include <thread>
#include <mutex>
#include <condition_variable>

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
    std::vector<int> priority;
    std::vector<std::vector<int>> movBuckets;
    for (auto &inst : InstArray)
    {
        priority.push_back(inst.first);
        inst.second->numMov = 0;
    }

    int num_iter = 30;
    int max_threads = 7;
    int HPWL_est = 1e8;
    for (int i = 0; i < num_iter; ++i)
    {
        std::cout << "Iteration " << i << "For LUT" << std::endl;
        ISMSolver_matching solver;
        std::vector<IndepSet> indepSets;
        // sort priority
        for (auto &bkt : movBuckets)
            bkt.clear();
        movBuckets.resize(num_iter);
        for (int i : priority)
        {
            movBuckets[InstArray[i]->numMov].push_back(i);
            // if(InstArray[i]->numMov>1)std::cout<<"mark";
        }
        auto it = priority.begin();
        for (const auto &bkt : movBuckets)
        {
            std::copy(bkt.begin(), bkt.end(), it);
            it += bkt.size();
        }
        solver.buildIndependentIndepSets(indepSets, 10, 50, 9, priority);
        std::cout << indepSets.size() << " independent sets." << std::endl;

        std::vector<std::thread> threads;
        std::mutex mtx;
        std::condition_variable cv;
        int active_threads = 0;
        for (auto &indepSet : indepSets)
        {
            {
                std::unique_lock<std::mutex> lock(mtx);
                cv.wait(lock, [&]() { return active_threads < max_threads; });
                ++active_threads;
            }

            threads.emplace_back(
                [&solver, &indepSet, &mtx, &cv, &active_threads]()
            {
                ISMMemory mem;
                indepSet.solution = solver.realizeMatching(mem, indepSet, 9);
                {
                    std::lock_guard<std::mutex> guard(mtx);
                    --active_threads;
                }
                cv.notify_one();
            }
            );


            // the fuction to be executed in non-parellel way :
            // ISMMemory mem;
            // indepSet.solution = solver.realizeMatching(mem, indepSet);
        }

        for (auto &thread : threads)
        {
            if (thread.joinable()) {
                thread.join();
            }
        }
        
        // std::cout << "Matching Complete." << std::endl;
        for (auto &indepSet : indepSets)
        {
            update_instance(indepSet);
        }
        int HPWL = update_net();
        if (HPWL_est - HPWL < 2 || (num_iter > 15 && HPWL_est - HPWL < 4)) break;
        else HPWL_est = HPWL;
    }
    std::cout << "Successfully realized matching." << std::endl;

    file_output(outputFileName);

    return 0;
}