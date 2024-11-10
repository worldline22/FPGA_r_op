#pragma once
#include <list>
#include <vector>
#include <string>
#include <map>
#include <set>
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
    // tile type 和 lib type 都改成数组，instanceMap的索引保留string
};

struct SInstance;

struct SPin
{
    int pinID; // 新增属性，用于管理PinArray，暂时不知道有没有用
    int netID; // 可能是0
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
    std::vector<SPin*> inpins;
    std::vector<SPin*> outpins;
    std::set<int> conn;
};

struct SNet
{
    int id;
    bool clock;
    SPin* inpin;
    std::list<SPin*> outpins;
};

extern std::map<int, SInstance*> InstArray;
extern std::map<int, SNet*> NetArray;
extern std::map<int, SPin*> PinArray;
extern std::vector<STile*> TileArray;  
// Tile Map 的索引规则: X是列有150个，Y是行有300个，这样子会产生150*300=45000个位置
// 45000*30

void init_tiles();
void copy_instances();
void copy_nets();
void connection_setup();

