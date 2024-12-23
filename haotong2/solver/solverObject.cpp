#include "solverObject.h"
#include <cassert>
#include <fstream>

std::map<int, SInstance*> InstArray;
std::map<int, SNet*> NetArray;
std::map<int, SPin*> PinArray;
std::vector<STile*> TileArray; 
SClockRegion ClockRegion_Info;

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

extern int index_2_z_inst(int index){
    return (index % 16) / 2;
}

extern int index_2_x_inst(int index)
{
    return (index / 16) % 150;
}

extern int index_2_y_inst(int index)
{
    return index / 2400;
}

void init_tiles()
{
    TileArray.resize(45000);
    for (int i = 0; i < 150; ++i)
    {
        for (int j = 0; j < 300; ++j)
        {
                // std::cout << i << ", " << j << "/ ";
            STile* tile = new STile();
            Tile* tileP = chip.getTile(i, j);
            tile->X = i;
            tile->Y = j;
            tile->type_add.clear();
            auto tileTypes = tileP->getTileTypes();
            if (tileTypes.size() == 0) tile->type = 0;
            else if (tileTypes.size() == 1)
            {
                std::string tileType = *tileTypes.begin();
                if (tileType == "PLB") tile->type = 1;
                else if (tileType == "DSP") tile->type = 2;
                else if (tileType == "RAMA") tile->type = 3;
                else if (tileType == "RAMB") tile->type = 4;
                else if (tileType == "GCLK") tile->type = 5;
                else if (tileType == "IOA") tile->type = 6;
                else if (tileType == "IOB") tile->type = 7;
                else if (tileType == "IPPIN") tile->type = 8;
            }
            else
            {
                tile->type = 9;
                for (auto type : tileTypes)
                {
                    if (type == "PLB") tile->type_add.push_back(1);
                    else if (type == "DSP") tile->type_add.push_back(2);
                    else if (type == "RAMA") tile->type_add.push_back(3);
                    else if (type == "RAMB") tile->type_add.push_back(4);
                    else if (type == "GCLK") tile->type_add.push_back(5);
                    else if (type == "IOA") tile->type_add.push_back(6);
                    else if (type == "IOB") tile->type_add.push_back(7);
                    else if (type == "IPPIN") tile->type_add.push_back(8);
                }
            }
            
            for (auto it = tileP->getInstanceMapBegin(); it != tileP->getInstanceMapEnd(); ++it)
            {
                std::string slotType = it->first;
                auto slotArr = it->second;
                std::vector<SSlot> tmpSlotArr;
                tmpSlotArr.resize(slotArr.size());
                // std::cout << slotType << " ";
                for (unsigned int i = 0; i < slotArr.size(); ++i)
                {
                    tmpSlotArr[i].baseline_InstIDs = slotArr[i]->getBaselineInstances();
                    tmpSlotArr[i].current_InstIDs = slotArr[i]->getBaselineInstances();
                }
                tile->instanceMap.insert(std::make_pair(slotType, tmpSlotArr));
            }
            // std::cout << std::endl;
            tile->netsConnected.clear();
            tile->netsConnected_bank0.clear();
            tile->pin_in_nets_bank0.clear();
            tile->netsConnected_bank1.clear();
            tile->pin_in_nets_bank1.clear();
            tile->has_fixed_bank0 = false;
            tile->has_fixed_bank1 = false;
            tile->CE_bank0.clear();
            tile->CE_bank1.clear();
            tile->RESET_bank0.clear();
            tile->RESET_bank1.clear();
            tile->CLOCK_bank0.clear();
            tile->CLOCK_bank1.clear();
            tile->seq_choose_num_bank0.resize(8, 0);
            tile->seq_choose_num_bank1.resize(8, 0);
            int index = xy_2_index(i, j);
            TileArray[index] = tile;
                // std::cout << index << ": " << tile->type << std::endl;
        }
    }
    for (int i = 0; i < 5; ++i)
    {
        ClockRegion_Info.xline[i] = chip.getClockRegion(i, 0)->getXRight();
        // std::cout << ClockRegion_Info.xline[i] << " ";
    }
    std::cout << std::endl;
    for (int i = 0; i < 5; ++i)
    {
        ClockRegion_Info.yline[i] = chip.getClockRegion(0, i)->getYTop();
        // std::cout << ClockRegion_Info.yline[i] << " ";
    }
    for (int i = 0; i < 25; ++i)
    {
        ClockRegion_Info.clockNets[i].clear();
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

        net->BBox_L = 150;
        net->BBox_R = 0;
        net->BBox_U = 0;
        net->BBox_D = 300;

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
            int tileIndex = xy_2_index(std::get<0>(instance->Location), std::get<1>(instance->Location));
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
        if (instance->Lib >= 9 && instance->Lib <= 15 && std::get<2>(instance->Location) >= 4) instance->bank = true;
        else if (instance->Lib == 19 && std::get<2>(instance->Location) >= 8) instance->bank = true;
        else if (instance->Lib == 0 && std::get<2>(instance->Location) == 1) instance->bank = true;
        else if (instance->Lib == 1 && std::get<2>(instance->Location) == 1) instance->bank = true;
        else instance->bank = false;

        if (instance->fixed){
            if(instance->bank){
                TileArray[tileIndex]->has_fixed_bank1 = true;
            }
            else{
                TileArray[tileIndex]->has_fixed_bank0 = true;
            }
        }
        
        // pins and nets
        instance->inpins.clear();
        for (int i = 0; i < instanceP.second->getNumInpins(); ++i)
        {
            SPin* pin = new SPin();
            pin->pinID = PinArray.size();
            PinArray.insert(std::make_pair(pin->pinID, pin));
            Pin* pin_old = instanceP.second->getInpin(i);
            pin->prop = pin_old->getProp();
            pin->timingCritical = pin_old->getTimingCritical();
            pin->instanceOwner = instance;
            pin->netID = pin_old->getNetID();
                // caution! maybe -1
                if (pin->netID != -1)
                {
                    auto netp = NetArray[pin->netID];
                    netp->outpins.push_back(pin);
                    // bounding box calc
                    netp->BBox_L = std::min(netp->BBox_L, std::get<0>(instance->Location));
                    netp->BBox_R = std::max(netp->BBox_R, std::get<0>(instance->Location));
                    netp->BBox_U = std::max(netp->BBox_U, std::get<1>(instance->Location));
                    netp->BBox_D = std::min(netp->BBox_D, std::get<1>(instance->Location));

                    // process related to netsConneted and pin_in_nets, control set
                    int findindex = 0;
                    int tileindex = xy_2_index(std::get<0>(instance->Location), std::get<1>(instance->Location));
                    // 需要构建新的tile的对应关系
                    auto tile_ptr = TileArray[tileindex];
                    if (!instance->bank)
                    {
                        tile_ptr->netsConnected.insert(pin->netID);
                        int sizen = tile_ptr->netsConnected_bank0.size();
                        for (findindex = 0; findindex < sizen; ++findindex)
                            if (tile_ptr->netsConnected_bank0[findindex] == pin->netID) break;
                        if (findindex == sizen)
                        {
                            tile_ptr->netsConnected_bank0.push_back(pin->netID);
                            tile_ptr->pin_in_nets_bank0.push_back(std::vector<int>{pin->pinID});
                        }
                        else
                            tile_ptr->pin_in_nets_bank0[findindex].push_back(pin->pinID);

                        if (pin->prop == PinProp::PIN_PROP_CLOCK)
                        {
                            if (tile_ptr->CLOCK_bank0.find(pin->netID) == tile_ptr->CLOCK_bank0.end())
                            {
                                tile_ptr->CLOCK_bank0.insert(pin->netID);
                            }
                        }
                        else if (pin->prop == PinProp::PIN_PROP_CE)
                        {
                            if (tile_ptr->CE_bank0.find(pin->netID) == tile_ptr->CE_bank0.end())
                            {
                                tile_ptr->CE_bank0.insert(pin->netID);
                            }
                        }
                        else if (pin->prop == PinProp::PIN_PROP_RESET)
                        {
                            if (tile_ptr->RESET_bank0.find(pin->netID) == tile_ptr->RESET_bank0.end())
                            {
                                tile_ptr->RESET_bank0.insert(pin->netID);
                            }
                        }
                    }
                    else
                    {
                        tile_ptr->netsConnected.insert(pin->netID);
                        int sizen = tile_ptr->netsConnected_bank1.size();
                        for (findindex = 0; findindex < sizen; ++findindex)
                            if (tile_ptr->netsConnected_bank1[findindex] == pin->netID) break;
                        if (findindex == sizen)
                        {
                            tile_ptr->netsConnected_bank1.push_back(pin->netID);
                            tile_ptr->pin_in_nets_bank1.push_back(std::vector<int>{pin->pinID});
                        }
                        else
                            tile_ptr->pin_in_nets_bank1[findindex].push_back(pin->pinID);

                        if (pin->prop == PinProp::PIN_PROP_CLOCK)
                        {
                            if (tile_ptr->CLOCK_bank1.find(pin->netID) == tile_ptr->CLOCK_bank1.end())
                            {
                                tile_ptr->CLOCK_bank1.insert(pin->netID);
                            }
                        }
                        else if (pin->prop == PinProp::PIN_PROP_CE)
                        {
                            if (tile_ptr->CE_bank1.find(pin->netID) == tile_ptr->CE_bank1.end())
                            {
                                tile_ptr->CE_bank1.insert(pin->netID);
                            }
                        }
                        else if (pin->prop == PinProp::PIN_PROP_RESET)
                        {
                            if (tile_ptr->RESET_bank1.find(pin->netID) == tile_ptr->RESET_bank1.end())
                            {
                                tile_ptr->RESET_bank1.insert(pin->netID);
                            }
                        }
                    }

                    if (pin->prop == PinProp::PIN_PROP_CLOCK && NetArray[pin->netID]->clock)
                    {
                        int x = std::get<0>(instance->Location);
                        int y = std::get<1>(instance->Location);
                        int CRID = ClockRegion_Info.getCRID(x, y);
                        ClockRegion_Info.clockNets[CRID].insert(pin->netID);
                    }
                }
                // std::cout << "in" << i << "(" << pin->netID << ", " << pin->prop << ", " << pin->timingCritical << ") ";
            instance->inpins.push_back(pin);
            // if (std::get<0>(instance->Location) <= 25)
            // {
            //     std::cout << "Input" << i << " " << pin->netID << " " << pin->prop << " " << pin->timingCritical << std::endl;
            // }
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
            pin->prop = pin_old->getProp();
            pin->timingCritical = pin_old->getTimingCritical();
            pin->instanceOwner = instance;
                if (pin->netID != -1) {
                    auto netp = NetArray[pin->netID];
                    assert(netp->inpin == nullptr);
                    netp->inpin = pin;
                    // bounding box calc
                    netp->BBox_L = std::min(netp->BBox_L, std::get<0>(instance->Location));
                    netp->BBox_R = std::max(netp->BBox_R, std::get<0>(instance->Location));
                    netp->BBox_U = std::max(netp->BBox_U, std::get<1>(instance->Location));
                    netp->BBox_D = std::min(netp->BBox_D, std::get<1>(instance->Location));

                    // process related to netsConneted and pin_in_nets
                    int findindex = 0;
                    int tileindex = xy_2_index(std::get<0>(instance->Location), std::get<1>(instance->Location));
                    // 这里关于Tile的信息需要更新，可能是一个产生bug的地方
                    auto tile_ptr = TileArray[tileindex];
                    if (!instance->bank)
                    {
                        tile_ptr->netsConnected.insert(pin->netID);
                        int sizen = tile_ptr->netsConnected_bank0.size();
                        for (findindex = 0; findindex < sizen; ++findindex)
                            if (tile_ptr->netsConnected_bank0[findindex] == pin->netID) break;
                        if (findindex == sizen)
                        {
                            tile_ptr->netsConnected_bank0.push_back(pin->netID);
                            tile_ptr->pin_in_nets_bank0.push_back(std::vector<int>{pin->pinID});
                        }
                        else
                            tile_ptr->pin_in_nets_bank0[findindex].push_back(pin->pinID);

                        if (pin->prop == PinProp::PIN_PROP_CLOCK)
                        {
                            if (tile_ptr->CLOCK_bank0.find(pin->netID) == tile_ptr->CLOCK_bank0.end())
                            {
                                tile_ptr->CLOCK_bank0.insert(pin->netID);
                            }
                        }
                        else if (pin->prop == PinProp::PIN_PROP_CE)
                        {
                            if (tile_ptr->CE_bank0.find(pin->netID) == tile_ptr->CE_bank0.end())
                            {
                                tile_ptr->CE_bank0.insert(pin->netID);
                            }
                        }
                        else if (pin->prop == PinProp::PIN_PROP_RESET)
                        {
                            if (tile_ptr->RESET_bank0.find(pin->netID) == tile_ptr->RESET_bank0.end())
                            {
                                tile_ptr->RESET_bank0.insert(pin->netID);
                            }
                        }
                    }
                    else
                    {
                        tile_ptr->netsConnected.insert(pin->netID);
                        int sizen = tile_ptr->netsConnected_bank1.size();
                        for (findindex = 0; findindex < sizen; ++findindex)
                            if (tile_ptr->netsConnected_bank1[findindex] == pin->netID) break;
                        if (findindex == sizen)
                        {
                            tile_ptr->netsConnected_bank1.push_back(pin->netID);
                            tile_ptr->pin_in_nets_bank1.push_back(std::vector<int>{pin->pinID});
                        }
                        else
                            tile_ptr->pin_in_nets_bank1[findindex].push_back(pin->pinID);

                        if (pin->prop == PinProp::PIN_PROP_CLOCK)
                        {
                            if (tile_ptr->CLOCK_bank1.find(pin->netID) == tile_ptr->CLOCK_bank1.end())
                            {
                                tile_ptr->CLOCK_bank1.insert(pin->netID);
                            }
                        }
                        else if (pin->prop == PinProp::PIN_PROP_CE)
                        {
                            if (tile_ptr->CE_bank1.find(pin->netID) == tile_ptr->CE_bank1.end())
                            {
                                tile_ptr->CE_bank1.insert(pin->netID);
                            }
                        }
                        else if (pin->prop == PinProp::PIN_PROP_RESET)
                        {
                            if (tile_ptr->RESET_bank1.find(pin->netID) == tile_ptr->RESET_bank1.end())
                            {
                                tile_ptr->RESET_bank1.insert(pin->netID);
                            }
                        }
                    }

                    if (pin->prop == PinProp::PIN_PROP_CLOCK && NetArray[pin->netID]->clock)
                    {
                        int x = std::get<0>(instance->Location);
                        int y = std::get<1>(instance->Location);
                        int CRID = ClockRegion_Info.getCRID(x, y);
                        ClockRegion_Info.clockNets[CRID].insert(pin->netID);
                    }
                }
                // std::cout << "out" << i << "(" << pin->netID << ", " << pin->prop << ", " << pin->timingCritical << ") ";
            instance->outpins.push_back(pin);
            // if (std::get<0>(instance->Location) <= 25)
            // {
            //     std::cout << "Output" << i << " " << pin->netID << " " << pin->prop << " " << pin->timingCritical << std::endl;
            // }
        }

        // if (std::get<0>(instance->Location) <= 25)
        // {
        //     std::cout << " > This is Instance " << instance->id << " " << instance->Lib << " " << instance->fixed << " " << std::get<0>(instance->Location) << " " << std::get<1>(instance->Location) << " " << std::get<2>(instance->Location) << " " << instance->inpins.size() << " " << instance->outpins.size() << std::endl;
        // }
        // std::cout << std::endl;
        
        InstArray.insert(std::make_pair(instance->id, instance));
        // std::cout << InstArray[instance->id]->id << " " << InstArray[instance->id]->Lib << " " << InstArray[instance->id]->fixed << " " << std::get<0>(InstArray[instance->id]->baseLocation) << " " << std::get<1>(InstArray[instance->id]->baseLocation) << " " << std::get<2>(InstArray[instance->id]->baseLocation) << InstArray[instance->id]->inpins.size() << " " << InstArray[instance->id]->outpins.size() << std::endl; 
        // check tile connect info
    }
    // for (int i = 0; i < 45000; ++i)
    //     {
    //         auto tile_ptr = TileArray[i]; 
    //         if (tile_ptr->type.find(0) == tile_ptr->type.end()) continue;
    //         std::cout << "Tile " << i << " nets connected - \n";
    //         std::cout << "bank0: ";
    //         for (int j = 0; j < (int)tile_ptr->netsConnected_bank0.size(); ++j)
    //         {
    //             int netID = tile_ptr->netsConnected_bank0[j];
    //             std::cout << "(net" << netID << ": ";
    //             for (int k = 0; k < (int)tile_ptr->pin_in_nets_bank0[j].size(); ++k)
    //             {
    //                 std::cout << tile_ptr->pin_in_nets_bank0[j][k] << " ";
    //             }
    //             std::cout << ")";
    //         }
    //         std::cout << std::endl << "bank1: ";
    //         for (int j = 0; j < (int)tile_ptr->netsConnected_bank1.size(); ++j)
    //         {
    //             int netID = tile_ptr->netsConnected_bank1[j];
    //             std::cout << "(net" << netID << ": ";
    //             for (int k = 0; k < (int)tile_ptr->pin_in_nets_bank1[j].size(); ++k)
    //             {
    //                 std::cout << tile_ptr->pin_in_nets_bank1[j][k] << " ";
    //             }
    //             std::cout << ")";
    //         }
    //         std::cout << std::endl;
    //     }

    // check clock region info
    // for (int i = 0; i < 25; ++i)
    // {
    //     std::cout << "Clock Region " << i << " has " << ClockRegion_Info.clockNets[i].size() << " clock nets: ";
    //     for (auto netID : ClockRegion_Info.clockNets[i])
    //     {
    //         std::cout << netID << " ";
    //     }
    //     std::cout << std::endl;
    // }
}

void connection_setup()
{
    for (auto netP : NetArray)
    {
        if (netP.second->clock) continue;
        std::vector<int> pinIDs{};
        pinIDs.push_back(netP.second->inpin->pinID);
        for (auto pin : netP.second->outpins)
        {
            pinIDs.push_back(pin->pinID);
        }
        int size = pinIDs.size();
        if (size > 15) continue;

        for (int i = 0; i < size; ++i)
        {
            for (int j = i + 1; j < size; ++j)
            {
                int inst1 = PinArray[pinIDs[i]]->instanceOwner->id;
                int inst2 = PinArray[pinIDs[j]]->instanceOwner->id;
                InstArray[inst1]->conn.insert(inst2);
                InstArray[inst2]->conn.insert(inst1);
            }
        }
    }
    
    // print the result
    // for (auto instP : InstArray)
    // {
    //     std::cout << "Instance " << instP.first << " is connected to: ";
    //     for (auto instID : instP.second->conn)
    //     {
    //         std::cout << instID << " ";
    //     }
    //     std::cout << std::endl;
    // }
}

void file_output(std::string filename)
{
    std::ofstream out(filename);
    for (auto instP : InstArray)
    {
        if (instP.first == -1) continue;
        int x = std::get<0>(instP.second->Location);
        int y = std::get<1>(instP.second->Location);
        int z = std::get<2>(instP.second->Location);
        int lib = instP.second->Lib;
        std::string libName;
        if (lib == 19) libName = "SEQ";
        else if (lib == 9) libName = "LUT1";
        else if (lib == 10) libName = "LUT2";
        else if (lib == 11) libName = "LUT3";
        else if (lib == 12) libName = "LUT4";
        else if (lib == 13) libName = "LUT5";
        else if (lib == 14) libName = "LUT6";
        else if (lib == 15) libName = "LUT6X";
        else if (lib == 0) libName = "CARRY4";
        else if (lib == 1) libName = "DRAM";
        else if (lib == 2) libName = "DSP";
        else if (lib == 3) libName = "F7MUX";
        else if (lib == 4) libName = "F8MUX";
        else if (lib == 5) libName = "GCLK";
        else if (lib == 6) libName = "IOA";
        else if (lib == 7) libName = "IOB";
        else if (lib == 8) libName = "IPPIN";
        else if (lib == 16) libName = "PLB";
        else if (lib == 17) libName = "RAMA";
        else if (lib == 18) libName = "RAMB";
        else libName = "UNKNOWN";
        bool fixed = instP.second->fixed;
        int id = instP.second->id;
        out << "X" << x;
        out << "Y" << y;
        out << "Z" << z;
        out << " " << libName << " inst_";
        out << id;
        if (fixed) out << " FIXED";
        out << std::endl;
    }
    
}