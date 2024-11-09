#pragma once
#include <list>
#include <vector>
#include <string>
#include <map>
#include <tuple>
#include <..\checker_legacy\object.h>
#include <..\checker_legacy\global.h>


extern int xy_2_index(int x, int y);
extern int index_2_x(int index);
extern int index_2_y(int index);



struct SSlot
{
    std::list<int> baseline_InstIDs;
    std::list<int> current_InstIDs;
};

struct STile
{
    int X; // col
    int Y; // row
    std::set<int> type; // M
    std::map<std::string, std::vector<SSlot*>> instanceMap;
};

struct SInstance;

struct SPin
{
    int netID;
    PinProp prop;
    bool timingCritical;
    SInstance* instanceOwner;
};

struct SInstance
{
    bool fixed;
    int Lib; // M
    int id;
    std::tuple<int, int, int> baseLocation;
    std::tuple<int, int, int> Location;
    std::vector<Pin*> inpins;
    std::vector<Pin*> outpins;
};

struct SNet
{
    int id;
    bool clock;
    Pin* inpin;
    std::list<Pin*> outpins;
};

extern std::map<int, SInstance*> InstArray;
extern std::map<int, SNet*> NetArray;
extern std::map<int, SPin*> PinArray;
extern std::vector<STile*> TileArray;  
// Tile Map 的索引规则: X是列有150个，Y是行有300个，这样子会产生150*300=45000个位置
// 45000*30

void init_tiles();
void copy_instances();

