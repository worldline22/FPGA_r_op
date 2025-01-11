#include "checker_legacy/global.h"
#include "checker_legacy/object.h"
#include "checker_legacy/lib.h"
#include "checker_legacy/arch.h"
#include "checker_legacy/netlist.h"

#include "solver/solverObject.h"
#include "solver/solver.h"
#include "solver/solverI.h"
#include "helper/debug.h"
#include <cassert>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <fstream>

#include<iostream>
#include<chrono>

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
    std::string dbinfo_file = "helper/dbinfo.log";
    std::ofstream dbinfo(dbinfo_file);

    std::ifstream config_file("config.txt");
    parse_config(config_file);

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
    pin_denMax = pindensity_setup();


    // for (auto tilep : TileArray)
    // {
    //     if (tilep->type != 1) continue;
    //     std::cout << "Tile " << tilep->X << " " << tilep->Y << " " << tilep->netsConnected_bank0.size() << " " << tilep->netsConnected_bank1.size() << std::endl;
    // }
    // std::set<int> press_test;
    // for (int i = 0; i < 45000; ++i)
    // {
    //     auto tile_ptr = TileArray[i];
    //     if (tile_ptr->type == 1)
    //     {
    //         press_test.insert(i);
    //     }
    // }
    // update_tile_I(press_test);
    // ForceArray.resize(InstArray.size());
    ForceArray.clear();
    for (int i = 0; i < int(InstArray.size()); ++i)
    {
        if (InstArray[i]->fixed) continue;
        if (!(InstArray[i]->Lib == 19 || (InstArray[i]->Lib >= 9 && InstArray[i]->Lib <= 15))) continue;
        Instance_Force_Pack* new_fp = new Instance_Force_Pack;
        new_fp->id = i;
        ForceArray.push_back(new_fp);
        // 这玩意tm有病吧，不能用构造函数，用了就会报莫名奇妙的segfault
    }
    // get_force();

    // solve start
    std::vector<int> priority;
    std::vector<std::vector<int>> movBuckets;
    for (auto &inst : InstArray)
    {
        priority.push_back(inst.first);
        inst.second->numMov = 0;
    }

    int overall_cost = 0;

    int num_iter = bank_iteration;
    int max_threads = 7;
    // int HPWL_est = 1e8;
    for (int i = 0; i < num_iter; ++i)
    {
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
            // if(InstArray[i]->numMov>1)std::cout<<"mark";
        }
        auto it = priority.begin();
        for (const auto &bkt : movBuckets)
        {
            std::copy(bkt.begin(), bkt.end(), it);
            it += bkt.size();
        }
        
        solver.buildIndependentIndepSets(indepSets, 10, 50, priority);
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
                indepSet.solution = solver.realizeMatching(mem, indepSet);
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

        int total_cost_sum = 0;
        for (int j = 0; j < int(indepSets.size()); ++j)
        {
            dbinfo << "Independent Set " << j << " : ";
            show_site_in_set(indepSets[j], dbinfo);
            total_cost_sum += indepSets[j].totalCost;
        }
        overall_cost += total_cost_sum;
        dbinfo << "Total Cost Sum: " << total_cost_sum << std::endl;
        
        // std::cout << "Matching Complete." << std::endl;
        for (auto &indepSet : indepSets)
        {
            update_instance(indepSet);
        }
        update_net();
        // int HPWL = update_net();
        // if (HPWL_est - HPWL < 2 || (num_iter > 15 && HPWL_est - HPWL < 3)) break;
        // else HPWL_est = HPWL;
    }
    std::cout << "Bank ISM finish." << std::endl;
    priority.clear();
    priority.resize(ForceArray.size());
    for (int i = 0; i < int(ForceArray.size()); ++i)
    {
        priority[i] = i;
    }
    for (auto &inst : InstArray)
    {
        inst.second->numMov = 0;
    }
    num_iter = instance_iteration;
    for (int i = 0; i < num_iter; ++i)
    {
        dbinfo << ">>>> IterationI " << i << std::endl;
        std::cout << "IterationI " << i << std::endl;
        get_force(i);

        sort(priority.begin(), priority.end(), [&](int a, int b) { return ForceArray[a]->F > ForceArray[b]->F; });
        for (int i = 0; i < int(ForceArray.size()); ++i)
        {
            dbinfo << priority[i] << " " << ForceArray[priority[i]]->F << std::endl;
        }
        std::vector<int> inst_priority;
        inst_priority.resize(ForceArray.size());
        for (int i = 0; i < int(ForceArray.size()); ++i)
        {
            inst_priority[i] = ForceArray[priority[i]]->id;
        }
        {
        ISMSolver_matching_I solver;
        std::vector<IndepSet> indepSets;
        // sort priority
        // for (auto &bkt : movBuckets)
        //     bkt.clear();
        // movBuckets.resize(100);
        // for (int i : priority)
        // {
        //     if (InstArray[i]->numMov>30) std::cout << InstArray[i]->numMov << "_______________________________________________________" << i << std::endl;
        //     movBuckets[InstArray[i]->numMov].push_back(i);
        //     // if(InstArray[i]->numMov>1)std::cout<<"mark";
        // }
        // auto it = priority.begin();
        // for (const auto &bkt : movBuckets)
        // {
        //     std::copy(bkt.begin(), bkt.end(), it);
        //     it += bkt.size();
        // }
        // auto start_prioroty = std::chrono::high_resolution_clock::now();
        
        // auto end_priority = std::chrono::high_resolution_clock::now();
        // std::chrono::duration<double> elapsed_priority = end_priority - start_prioroty;
        // std::cout << "Priority Sort Time: " << elapsed_priority.count() << "s" << std::endl;


        // auto start_LUT_build = std::chrono::high_resolution_clock::now();
        solver.buildIndependentIndepSets(indepSets, 15, 100, 9, inst_priority);
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
                indepSet.solution = solver.realizeMatching_Instance(mem, indepSet, 9);
                // indepSet.totalCost = mem.totalCost;
                {
                    std::lock_guard<std::mutex> guard(mtx);
                    --active_threads;
                }
                cv.notify_one();
            }
            );
        }
        for (auto &thread : threads)
        {
            if (thread.joinable()) {
                thread.join();
            }
        }
        int total_cost_sum = 0;
        for (int j = 0; j < int(indepSets.size()); ++j)
        {
            dbinfo << "Independent Set " << j << " : ";
            show_site_in_set(indepSets[j], dbinfo);
            total_cost_sum += indepSets[j].totalCost;
        }
        overall_cost += total_cost_sum;
        dbinfo << "Total Cost Sum: " << total_cost_sum << std::endl;
        dbinfo << "----------------------------------------------------LUT Search Over----------------------------------------------------" << std::endl;
        // auto end_LUT_build = std::chrono::high_resolution_clock::now();
        // std::chrono::duration<double> elapsed_LUT_build = end_LUT_build - start_LUT_build;
        // std::cout << "LUT Build Time: " << elapsed_LUT_build.count() << "s" << std::endl;

        // auto start_updateLUT = std::chrono::high_resolution_clock::now();
        std::set<int> changed_tiles;
        for (auto &indepSet : indepSets)
        {
            auto changed = update_instance_I(indepSet, 1);
            changed_tiles.insert(changed.begin(), changed.end());
        }
        update_tile_I(changed_tiles);
        update_net();
        // auto end_updateLUT = std::chrono::high_resolution_clock::now();
        // std::chrono::duration<double> elapsed_updateLUT = end_updateLUT - start_updateLUT;
        // std::cout << "LUT Update Time: " << elapsed_updateLUT.count() << "s" << std::endl;
        }

        {
        ISMSolver_matching_I solver;
        std::vector<IndepSet> indepSets;
        // sort priority
        for (auto &bkt : movBuckets)
            bkt.clear();
        movBuckets.resize(100);
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
        // auto start_SEQ_build = std::chrono::high_resolution_clock::now();
        // auto start_SEQ_build1 = std::chrono::high_resolution_clock::now();
        solver.buildIndependentIndepSets(indepSets, 15, 100, 19, inst_priority);
        // auto end_SEQ_build1 = std::chrono::high_resolution_clock::now();
        // std::chrono::duration<double> elapsed_SEQ_build1 = end_SEQ_build1 - start_SEQ_build1;
        // std::cout << "SEQ Build Time1: " << elapsed_SEQ_build1.count() << "s" << std::endl;

        std::cout << indepSets.size() << " independent sets." << std::endl;
        std::vector<std::thread> threads;
        std::mutex mtx;
        std::condition_variable cv;
        int active_threads = 0;
        // auto start_matching_count = std::chrono::high_resolution_clock::now();
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
                indepSet.solution = solver.realizeMatching_Instance(mem, indepSet, 19);
                {
                    std::lock_guard<std::mutex> guard(mtx);
                    --active_threads;
                }
                cv.notify_one();
            }
            );
        }
        for (auto &thread : threads)
        {
            if (thread.joinable()) {
                thread.join();
            }
        }
        std::cout << "Matching Complete." << std::endl;
        // auto end_matching_count = std::chrono::high_resolution_clock::now();
        // std::chrono::duration<double> elapsed_matching_count = end_matching_count - start_matching_count;
        // std::cout << "SEQ Matching Count Time: " << elapsed_matching_count.count() << "s" << std::endl;
        int total_cost_sum = 0;
        for (int j = 0; j < int(indepSets.size()); ++j)
        {
            dbinfo << "Independent Set " << j << " : ";
            show_site_in_set(indepSets[j], dbinfo);
            total_cost_sum += indepSets[j].totalCost;
        }
        overall_cost += total_cost_sum;
        dbinfo << "Total Cost Sum: " << total_cost_sum << std::endl;
        dbinfo << "----------------------------------------------------SEQ Search Over----------------------------------------------------" << std::endl;
        // auto end_SEQ_build = std::chrono::high_resolution_clock::now();
        // std::chrono::duration<double> elapsed_SEQ_build = end_SEQ_build - start_SEQ_build;
        // std::cout << "SEQ Build Time: " << elapsed_SEQ_build.count() << "s" << std::endl;

        // auto start_updateSEQ = std::chrono::high_resolution_clock::now();
        std::set<int> changed_tiles;
        for (auto &indepSet : indepSets)
        {
            auto changed = update_instance_I(indepSet, 2);
            changed_tiles.insert(changed.begin(), changed.end());
        }
        update_tile_I(changed_tiles);
        update_net();
        // auto end_updateSEQ = std::chrono::high_resolution_clock::now();
        // std::chrono::duration<double> elapsed_updateSEQ = end_updateSEQ - start_updateSEQ;
        // std::cout << "SEQ Update Time: " << elapsed_updateSEQ.count() << "s" << std::endl;
        }
    }
    std::cout << "Instance SEQ ISM finish." << std::endl;
    std::cout << "Overall Cost: " << overall_cost << std::endl;

    // after instanceISM:
    // for, update_instance_I
    // update_tiles_I
    // update_net

    // for (auto &inst : InstArray)
    // {
    //     inst.second->numMov = 0;
    // }
    // num_iter = 20;
    // for (int i = 0; i < num_iter; ++i)
    // {
    //     std::cout<< "test" << std::endl;
    //     std::cout << "IterationI " << i << std::endl;
    //     {
    //     ISMSolver_matching_I solver;
    //     std::vector<IndepSet> indepSets;
    //     // sort priority
    //     for (auto &bkt : movBuckets)
    //         bkt.clear();
    //     movBuckets.resize(num_iter*2);
    //     for (int i : priority)
    //     {
    //         movBuckets[InstArray[i]->numMov].push_back(i);
    //         // if(InstArray[i]->numMov>1)std::cout<<"mark";
    //     }
    //     auto it = priority.begin();
    //     for (const auto &bkt : movBuckets)
    //     {
    //         std::copy(bkt.begin(), bkt.end(), it);
    //         it += bkt.size();
    //     }
    //     solver.buildIndependentIndepSets(indepSets, 10, 50, 9, priority);
    //     std::cout << indepSets.size() << " independent sets." << std::endl;
        
    //     std::vector<std::thread> threads;
    //     std::mutex mtx;
    //     std::condition_variable cv;
    //     int active_threads = 0;
    //     for (auto &indepSet : indepSets)
    //     {
    //         {
    //             std::unique_lock<std::mutex> lock(mtx);
    //             cv.wait(lock, [&]() { return active_threads < max_threads; });
    //             ++active_threads;
    //         }

    //         threads.emplace_back(
    //             [&solver, &indepSet, &mtx, &cv, &active_threads]()
    //         {
    //             ISMMemory mem;
    //             indepSet.solution = solver.realizeMatching_Instance(mem, indepSet, 9);
    //             {
    //                 std::lock_guard<std::mutex> guard(mtx);
    //                 --active_threads;
    //             }
    //             cv.notify_one();
    //         }
    //         );
    //     }
    //     for (auto &thread : threads)
    //     {
    //         if (thread.joinable()) {
    //             thread.join();
    //         }
    //     }
    //     std::set<int> changed_tiles;
    //     for (auto &indepSet : indepSets)
    //     {
    //         auto changed = update_instance_I(indepSet, 1);
    //         changed_tiles.insert(changed.begin(), changed.end());
    //     }
    //     update_tile_I(changed_tiles);
    //     update_net();
    //     }

    //     {
    //     ISMSolver_matching_I solver;
    //     std::vector<IndepSet> indepSets;
    //     // sort priority
    //     for (auto &bkt : movBuckets)
    //         bkt.clear();
    //     movBuckets.resize(num_iter*2);
    //     for (int i : priority)
    //     {
    //         movBuckets[InstArray[i]->numMov].push_back(i);
    //         // if(InstArray[i]->numMov>1)std::cout<<"mark";
    //     }
    //     auto it = priority.begin();
    //     for (const auto &bkt : movBuckets)
    //     {
    //         std::copy(bkt.begin(), bkt.end(), it);
    //         it += bkt.size();
    //     }
    //     solver.buildIndependentIndepSets(indepSets, 10, 50, 9, priority);
    //     std::cout << indepSets.size() << " independent sets." << std::endl;

    //     std::vector<std::thread> threads;
    //     std::mutex mtx;
    //     std::condition_variable cv;
    //     int active_threads = 0;
    //     for (auto &indepSet : indepSets)
    //     {
    //         {
    //             std::unique_lock<std::mutex> lock(mtx);
    //             cv.wait(lock, [&]() { return active_threads < max_threads; });
    //             ++active_threads;
    //         }

    //         threads.emplace_back(
    //             [&solver, &indepSet, &mtx, &cv, &active_threads]()
    //         {
    //             ISMMemory mem;
    //             indepSet.solution = solver.realizeMatching_Instance(mem, indepSet, 9);
    //             {
    //                 std::lock_guard<std::mutex> guard(mtx);
    //                 --active_threads;
    //             }
    //             cv.notify_one();
    //         }
    //         );
    //     }
    //     for (auto &thread : threads)
    //     {
    //         if (thread.joinable()) {
    //             thread.join();
    //         }
    //     }
    //     std::set<int> changed_tiles;
    //     for (auto &indepSet : indepSets)
    //     {
    //         auto changed = update_instance_I(indepSet, 1);
    //         changed_tiles.insert(changed.begin(), changed.end());
    //     }
    //     update_tile_I(changed_tiles);
    //     update_net();
    //     }
    // }
    // std::cout << "Instance LUT ISM finish." << std::endl;

    // after instanceISM:
    // for, update_instance_I
    // update_tiles_I
    // update_net

    file_output(outputFileName);
    std::cout << "Output file generated." << std::endl;

    return 0;
}

// 现在有个大大大大的问题，就是为什么inst_0的位置总是不对，很器官。
