#include "debug.h"
#include <iostream>

void show_site_in_set(IndepSet const &set, std::ostream &out)
{
    // out << "Independent Set:";
    if (set.type == 1) out << " LUT" << std::endl;
    else if (set.type == 2) out << " SEQ" << std::endl;

    int totalCost = set.totalCost;

    for (int i = 0; i < int(set.inst.size()); i++)
    {
        int inst_site = set.inst[i];
        int pre_x = index_2_x_inst(inst_site);
        int pre_y = index_2_y_inst(inst_site);
        int pre_z = inst_site % 16;
        int solution_site = set.inst[set.solution[i]];
        int post_x = index_2_x_inst(solution_site);
        int post_y = index_2_y_inst(solution_site);
        int post_z = solution_site % 16;

        // get instance of the site
        int tile_id = xy_2_index(pre_x, pre_y);
        STile *tile = TileArray[tile_id];
        SInstance *inst = nullptr;
        if (set.type == 1) {
            std::list<int> &instIDs = tile->instanceMap["LUT"][pre_z/2].current_InstIDs;
            if (instIDs.size() == 0) inst = nullptr;
            else if (instIDs.size() == 1) inst = InstArray[*instIDs.begin()];
            else if (instIDs.size() == 2){
                if (pre_z % 2 == 0) inst = InstArray[*instIDs.begin()];
                else inst = InstArray[*instIDs.rbegin()];
            }
        }
        else if (set.type == 2) {
            std::list<int> &instIDs = tile->instanceMap["SEQ"][pre_z].current_InstIDs;
            if (instIDs.size() == 0) inst = nullptr;
            else inst = InstArray[*instIDs.begin()];
            assert(instIDs.size() == 1 || instIDs.size() == 0);
        }
        int inst_id;
        if (inst != nullptr) inst_id = inst->id;
        else inst_id = -1;
        out << "Site " << i << " | " << "("  << pre_x << ", " << pre_y << ", " << pre_z << ") -> (" << post_x << ", " << post_y << ", " << post_z << ") | Instance:" << inst_id << std::endl;
        
            out << "PartCost: " << set.partCost[i] << std::endl;
        
    }

    out << "Total Cost: " << totalCost << std::endl;
}