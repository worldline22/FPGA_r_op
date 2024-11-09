#include "solverObject.h"

extern int xy_2_index(int x, int y)
{
    return y * 150 + x;
}

extern int index_2_x(int index)
{
    return index % 150;
}

extern int index_2_y(int index)
{
    return index / 150;
}

void copy_instances()
{
    for (auto instanceP : glbInstMap)
    {
        SInstance* instance = new SInstance();
            // std::cout << "Instance name: " << instanceP.second->getInstanceName() << std::endl;
        instance->baseLocation = instanceP.second->getBaseLocation();
        instance->Location = instance->baseLocation;
            // std::cout << "baseLocation: " << std::get<0>(instance->baseLocation) << " " << std::get<1>(instance->baseLocation) << " " << std::get<2>(instance->baseLocation);
        instance->fixed = instanceP.second->isFixed();
        instance->id = instanceP.first;
            // std::cout << " id: " << instance->id << std::endl;

        std::string modelName = instanceP.second->getModelName();
        if (modelName == "CARRY4") instance->Lib = 0;
        else if (modelName == "DRAM") instance->Lib = 1;
        else if (modelName == "DSP") instance->Lib = 2;
        else if (modelName == "F7MUX") instance->Lib = 3;
        else if (modelName == "F8MUX") instance->Lib = 4;
        else if (modelName == "GCLK") instance->Lib = 5;
        else if (modelName == "IOA") instance->Lib = 6;
        else if (modelName == "IOB") instance->Lib = 7;
        else if (modelName == "IPPIN") instance->Lib = 8;
        else if (modelName == "LUT1") instance->Lib = 9;
        else if (modelName == "LUT2") instance->Lib = 10;
        else if (modelName == "LUT3") instance->Lib = 11;
        else if (modelName == "LUT4") instance->Lib = 12;
        else if (modelName == "LUT5") instance->Lib = 13;
        else if (modelName == "LUT6") instance->Lib = 14;
        else if (modelName == "LUT6X") instance->Lib = 15;
        else if (modelName == "PLB") instance->Lib = 16;
        else if (modelName == "RAMA") instance->Lib = 17;
        else if (modelName == "RAMB") instance->Lib = 18;
        else if (modelName == "SEQ") instance->Lib = 19;
        else instance->Lib = -1;
            // std::cout << "Lib: " << instance->Lib << std::endl;
    }
}