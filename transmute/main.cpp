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
    std::cout << "Instance Moveable: " << ForceArray.size() << std::endl;

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
        if (dbinfo_enable)
        {
            int total_cost_sum = 0;
            for (int j = 0; j < int(indepSets.size()); ++j)
            {
                dbinfo << "Independent Set " << j << " : ";
                show_site_in_set(indepSets[j], dbinfo);
                total_cost_sum += indepSets[j].totalCost;
            }
            overall_cost += total_cost_sum;
            dbinfo << "Total Cost Sum: " << total_cost_sum << std::endl;
        }
        
        // std::cout << "Matching Complete." << std::endl;
        for (auto &indepSet : indepSets)
        {
            update_instance(indepSet);
        }
        update_net();
    }
    std::cout << "Bank ISM finish." << std::endl;
    pin_denMax = pin_denMax * 2;
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
    const int MaxR = indepSet_radius;
    const int MaxIndepSetSize = indepSet_volume;
    MaxIndepSetNum = ForceArray.size() / 300;
    MaxEmptyNum = MaxIndepSetSize - 10;
    for (int i = 0; i < num_iter; ++i)
    {
        if (dbinfo_enable) dbinfo << ">>>> IterationI " << i << std::endl;
        std::cout << "IterationI " << i << std::endl;
        get_force(i);

        sort(priority.begin(), priority.end(), [&](int a, int b) { return ForceArray[a]->F > ForceArray[b]->F; });
        
        std::vector<int> inst_priority;
        inst_priority.resize(ForceArray.size());
        for (int i = 0; i < int(ForceArray.size()); ++i)
        {
            inst_priority[i] = ForceArray[priority[i]]->id;
        }
        {
        ISMSolver_matching_I solver;
        std::vector<IndepSet> indepSets;
        
        solver.buildIndependentIndepSets(indepSets, MaxR, MaxIndepSetSize, 9, inst_priority);
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
        if (dbinfo_enable)
        {
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
        }
        
        std::set<int> changed_tiles;
        for (auto &indepSet : indepSets)
        {
            auto changed = update_instance_I(indepSet, 1);
            changed_tiles.insert(changed.begin(), changed.end());
        }
        update_tile_I(changed_tiles);
        update_net();
        }

        {
        ISMSolver_matching_I solver;
        std::vector<IndepSet> indepSets;
        solver.buildIndependentIndepSets(indepSets, MaxR, MaxIndepSetSize, 19, inst_priority);

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
        if (dbinfo_enable)
        {
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
        }
        std::set<int> changed_tiles;
        for (auto &indepSet : indepSets)
        {
            auto changed = update_instance_I(indepSet, 2);
            changed_tiles.insert(changed.begin(), changed.end());
        }
        update_tile_I(changed_tiles);
        update_net();
        }
    }
    std::cout << "Instance SEQ ISM finish." << std::endl;
    // std::cout << "Overall Cost: " << overall_cost << std::endl;


    file_output(outputFileName);
    std::cout << "Output file generated." << std::endl;

    return 0;
}

