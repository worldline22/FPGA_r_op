#include "solverObject.h"
#include <cassert>

std::map<int, SInstance*> InstArray;
std::map<int, SNet*> NetArray;
std::map<int, SPin*> PinArray;
std::vector<STile*> TileArray; 

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

void init_tiles()
{
    TileArray.resize(45000);
    for (int i = 0; i < 150; ++i)
    {
        for (int j = 0; j < 300; ++j)
        {
                // std::cout << i << ", " << j << ": ";
            STile* tile = new STile();
            Tile* tileP = chip.getTile(i, j);
            tile->X = i;
            tile->Y = j;
            auto tileTypes = tileP->getTileTypes();
            for (auto type : tileTypes)
            {
                // std::cout << type << " ";
                if (type == "PLB") tile->type.insert(0);
                else if (type == "DSP") tile->type.insert(1);
                else if (type == "RAMA") tile->type.insert(2);
                else if (type == "RAMB") tile->type.insert(3);
                else if (type == "GCLK") tile->type.insert(4);
                else if (type == "IOA") tile->type.insert(5);
                else if (type == "IOB") tile->type.insert(6);
                else if (type == "IPPIN") tile->type.insert(7);
                // else std::cout << "Error!";
            }
            for (auto it = tileP->getInstanceMapBegin(); it != tileP->getInstanceMapEnd(); ++it)
            {
                auto slotType = it->first;
                auto slotArr = it->second;
                std::vector<SSlot*> tmpSlotArr;
                // std::cout << slotType << " ";
                for (unsigned int i = 0; i < slotArr.size(); ++i)
                {
                    SSlot* slot = new SSlot();
                    slot->baseline_InstIDs = slotArr[i]->getBaselineInstances();
                    slot->current_InstIDs = slotArr[i]->getOptimizedInstances();
                    tmpSlotArr.push_back(slot);
                }
                tile->instanceMap[slotType] = tmpSlotArr;
            }
            // std::cout << std::endl;
            int index = xy_2_index(i, j);
            TileArray[index] = tile;
        }
    }
}

void copy_nets()
{
    NetArray.clear();
    for (auto netP : glbNetMap)
    {
        SNet* net = new SNet();
            // std::cout << "Net ID: " << netP.first << std::endl;
        net->id = netP.first;
        assert(netP.second->getId() == netP.first);
        net->clock = netP.second->isClock();
            // std::cout << "Clock: " << net->clock << std::endl;
        // 这里: 无法从Net信息找到Pin是Instance的哪个端口，也就无从知道具体是哪个pin. 因此先建net再建pin
        net->inpin = nullptr;
        net->outpins.clear();

        NetArray.insert(std::make_pair(net->id, net));
    }
}

void copy_instances()
{
    InstArray.clear();
    PinArray.clear();
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

        // check the instance position
        // int x = std::get<0>(instance->Location);
        // int y = std::get<1>(instance->Location);
        // int index = xy_2_index(x, y);
        // int z= std::get<2>(instance->Location);
        // if (instance->Lib >= 9 && instance->Lib <= 15)
        // {
        //     STile* tile_ptr = TileArray[index];
        //     SSlot* slot_ptr = tile_ptr->instanceMap["LUT"][z];
        //     bool found = false;
        //     for (auto instID : slot_ptr->baseline_InstIDs)
        //     {
        //         if (instID == instance->id)
        //         {
        //             found = true;
        //             break;
        //         }
        //     }
        //     assert(found);
        //     // success
        // }

        instance->inpins.clear();
        for (int i = 0; i < instanceP.second->getNumInpins(); ++i)
        {
            SPin* pin = new SPin();
            pin->pinID = PinArray.size();
            PinArray.insert(std::make_pair(pin->pinID, pin));
            Pin* pin_old = instanceP.second->getInpin(i);
            pin->netID = pin_old->getNetID();
                // caution! maybe -1
                if (pin->netID != -1)
                {
                    NetArray[pin->netID]->outpins.push_back(pin);
                }
            pin->prop = pin_old->getProp();
            pin->timingCritical = pin_old->getTimingCritical();
            pin->instanceOwner = instance;
                // std::cout << "in" << i << "(" << pin->netID << ", " << pin->prop << ", " << pin->timingCritical << ") ";
            instance->inpins.push_back(pin);
        }
        // std::cout << std::endl;

        instance->outpins.clear();
        for (int i = 0; i < instanceP.second->getNumOutpins(); ++i)
        {
            SPin* pin = new SPin();
            pin->pinID = PinArray.size();
            PinArray.insert(std::make_pair(pin->pinID, pin));
            Pin* pin_old = instanceP.second->getOutpin(i);
            pin->netID = pin_old->getNetID();
                if (pin->netID != -1) {
                    assert(NetArray[pin->netID]->inpin == nullptr);
                    NetArray[pin->netID]->inpin = pin;
                }
            pin->prop = pin_old->getProp();
            pin->timingCritical = pin_old->getTimingCritical();
            pin->instanceOwner = instance;
                // std::cout << "out" << i << "(" << pin->netID << ", " << pin->prop << ", " << pin->timingCritical << ") ";
            instance->outpins.push_back(pin);
        }
        // std::cout << std::endl;
        
        InstArray.insert(std::make_pair(instance->id, instance));
        // std::cout << InstArray[instance->id]->id << " " << InstArray[instance->id]->Lib << " " << InstArray[instance->id]->fixed << " " << std::get<0>(InstArray[instance->id]->baseLocation) << " " << std::get<1>(InstArray[instance->id]->baseLocation) << " " << std::get<2>(InstArray[instance->id]->baseLocation) << InstArray[instance->id]->inpins.size() << " " << InstArray[instance->id]->outpins.size() << std::endl; 
    }
}