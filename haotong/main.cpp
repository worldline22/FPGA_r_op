#include "checker_legacy/global.h"
#include "checker_legacy/object.h"
#include "checker_legacy/lib.h"
#include "checker_legacy/arch.h"
#include "checker_legacy/netlist.h"

#include "solver/solverObject.h"
#include "solver/solver.h"
#include <cassert>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>

int main(int, char* argv[])
{
    std::string nodeFileName = argv[1];
    std::string netFileName = argv[2];
    std::string timingFileName = argv[3];
    std::string outputFileName = argv[4]; 

    std::string libFileName = "../benchmark/fpga.lib";
    std::string sclFileName = "../benchmark/fpga.scl";
    std::string clkFileName = "../benchmark/fpga.clk";

    readAndCreateLib(libFileName);
    chip.readArch(sclFileName, clkFileName);
    readInputNodes(nodeFileName);
    readInputNets(netFileName);
    readInputTiming(timingFileName);
    std::cout << "Successfully read design files." << std::endl;

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

    int num_iter = 10;
    int max_threads = 8;

    for (int i = 0; i < num_iter; ++i)
    {
        ISMMemory mem;
        std::cout << "Iteration " << i << std::endl;
        ISMSolver_matching solver;
        std::vector<IndepSet> indepSets;

        // sort priority
        for (auto &bkt : movBuckets)
            bkt.clear();
        movBuckets.resize(num_iter);
        for (int i : priority)
        {
            movBuckets[InstArray[i]->numMov].push_back(i);
        }
        auto it = priority.begin();
        for (const auto &bkt : movBuckets)
        {
            std::copy(bkt.begin(), bkt.end(), it);
            it += bkt.size();
        }
        
        solver.buildIndependentIndepSets(indepSets, 10, 50, priority);

        std::vector<std::thread> threads;
        std::mutex mtx;
        std::condition_variable cv;
        int active_threads = 0;

        // for (auto &indepSet : indepSets)
        // {   
        //     {
        //         // Wait until the number of active threads is below the maximum limit
        //         std::unique_lock<std::mutex> lock(mtx);
        //         cv.wait(lock, [&]() { return active_threads < max_threads; });

        //         ++active_threads;
        //     }

        //     IndepSet thread_indepSet = indepSet; // copy of indepSet to prevent different threads from modifying the same object

        //     threads.emplace_back([&solver, thread_indepSet, &mtx, &cv, &active_threads]() mutable {
        //         ISMMemory thread_mem;  // create ISMMemory instance inside to prevent different threads from modifying the same object
        //         solver.realizeMatching(thread_mem, thread_indepSet);

        //         {
        //             std::lock_guard<std::mutex> guard(mtx);
        //             --active_threads;
        //         }
        //         cv.notify_one();
        //     });
        // }

        for (auto &indepSet : indepSets)
        {   
            {
                // Wait until the number of active threads is below the maximum limit
                std::unique_lock<std::mutex> lock(mtx);
                cv.wait(lock, [&]() { return active_threads < max_threads; });

                ++active_threads;
            }

            threads.emplace_back([&solver, &mem, &indepSet, &mtx, &cv, &active_threads]() {
                ISMMemory thread_mem;
                solver.realizeMatching(thread_mem, indepSet);

                {
                    std::lock_guard<std::mutex> guard(mtx);
                    --active_threads;
                }
                cv.notify_one();
            });
        }

        // Wait for all threads to complete
        for (auto &thread : threads)
        {
            if (thread.joinable()) {
                thread.join();
            }
        }
        
        // check the instance position
        for (auto instancepair : InstArray)
        {
            auto instance=instancepair.second;
            int x = std::get<0>(instance->Location);
            int y = std::get<1>(instance->Location);
            int index = xy_2_index(x, y);
            int z = std::get<2>(instance->Location);
            if (instance->Lib >= 9 && instance->Lib <= 15)
            {
                STile* tile_ptr = TileArray[index];
                bool found = false;
                // std::cout << "instance id: " << instance->id << " / ";
                for (auto instID : tile_ptr->instanceMap["LUT"][z].current_InstIDs)
                {
                    // std::cout << instID << " ";
                    if (instID == instance->id)
                    {
                        found = true;
                        break;
                    }
                }
                // std::cout << std::endl;
                if (!found)
                {std::cout << "not";
                std::cout << "found";
                }
                assert(found);
                // success
            }
        } 
    }

    std::cout << "Successfully realized matching." << std::endl;

    file_output(outputFileName);

    return 0;
}