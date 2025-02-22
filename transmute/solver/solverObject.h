#pragma once
#include <list>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <tuple>
#include "../checker_legacy/object.h"
#include "../checker_legacy/global.h"


extern int xy_2_index(int x, int y);
extern int index_2_x(int index);
extern int index_2_y(int index);
extern int index_2_x_inst(int index);
extern int index_2_y_inst(int index);
extern int index_2_z_inst(int index);



// struct SSlot
// {
//     std::list<int> baseline_InstIDs;
//     std::list<int> current_InstIDs;
// };

struct SSlot {
    std::list<int> baseline_InstIDs;
    std::list<int> current_InstIDs;
};

struct STile
{
    int X; // col
    int Y; // row
    int type;
    std::vector<int> type_add;
    std::map<std::string, std::vector<SSlot>> instanceMap;
    // tile type 和 lib type 都改成数组，instanceMap的索引保留string
    std::set<int> netsConnected;
    std::vector<int> netsConnected_bank0;
    std::vector<std::vector<int>> pin_in_nets_bank0;
    std::vector<int> netsConnected_bank1;
    std::vector<std::vector<int>> pin_in_nets_bank1;
    bool has_fixed_bank0;
    bool has_fixed_bank1;
    std::set<int> CE_bank0;
    std::set<int> CE_bank1;
    std::set<int> RESET_bank0;
    std::set<int> RESET_bank1;
    std::set<int> CLOCK_bank0;
    std::set<int> CLOCK_bank1;
    std::vector<int> seq_choose_num_bank0;
    std::vector<int> seq_choose_num_bank1;
    int pin_density;
    // 需要一个getControlSet()
    // 如果默认是其他类，默认放在'bank0'里面，PLB里的CARRY4，DRAM，DFF和LUT类引脚则可能放在bank1里面

    // 深复制构造函数
    // STile(const STile& other) {
    //     X = other.X;
    //     Y = other.Y;
    //     type = other.type;
    //     for (const auto& pair : other.instanceMap) {
    //         std::vector<SSlot*> vec;
    //         for (const auto& slot : pair.second) {
    //             vec.push_back(new SSlot(*slot)); // 深复制每个 SSlot 对象
    //         }
    //         instanceMap[pair.first] = vec;
    //     }
    // }

    // // 深复制赋值运算符
    // STile& operator=(const STile& other) {
    //     if (this != &other) {
    //         X = other.X;
    //         Y = other.Y;
    //         type = other.type;
    //         for (auto& pair : instanceMap) {
    //             for (auto& slot : pair.second) {
    //                 delete slot; // 释放旧的 SSlot 对象
    //             }
    //         }
    //         instanceMap.clear();
    //         for (const auto& pair : other.instanceMap) {
    //             std::vector<SSlot*> vec;
    //             for (const auto& slot : pair.second) {
    //                 vec.push_back(new SSlot(*slot)); // 深复制每个 SSlot 对象
    //             }
    //             instanceMap[pair.first] = vec;
    //         }
    //     }
    //     return *this;
    // }

    // // 析构函数
    // ~STile() {
    //     for (auto& pair : instanceMap) {
    //         for (auto& slot : pair.second) {
    //             delete slot; // 释放动态分配的 SSlot 对象
    //         }
    //     }
    // }
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
    bool bank;
    std::vector<SPin*> inpins;
    std::vector<SPin*> outpins;
    std::set<int> conn;
    int numMov = 0;
};

struct Instance_Force_Pack
{
    int id;
    float F_leave_x, F_leave_y;
    float F_stay;
    float F;
};

extern std::vector<Instance_Force_Pack*> ForceArray;

struct SNet
{
    int id;
    bool clock;
    SPin* inpin;
    std::list<SPin*> outpins;
    int BBox_L; // x方向
    int BBox_R;
    int BBox_U; // y方向
    int BBox_D;
    
};

struct SClockRegion
{
    int xline[5];
    int yline[5];
    //共有X0Y0-X4Y4共25个区域，索引方式遵从与TileArray相同的规则，索引为0-24
    // index = y * 5 + x
    // x = index % 5
    // y = index / 5
    std::set<int> clockNets[25];
    // clockNet in each region should not be more than 28

    // X轴方向，0-25,26-53,54-85,86-113,114-149
    // Y轴方向，0-59,60-119,120-179，180-239，240-299

    // setup in init_tiles, write value in copy_instances

    int getCRID(int tileindex_x, int tileindex_y)
    {
        int crindex_x, crindex_y;
        for (int i = 0; i < 5; ++i)
        {
            if (tileindex_x <= xline[i])
            {
                crindex_x = i;
                break;
            }
        }
        for (int i = 0; i < 5; ++i)
        {
            if (tileindex_y <= yline[i])
            {
                crindex_y = i;
                break;
            }
        }
        return crindex_y * 5 + crindex_x;
    }
};

extern std::map<int, SInstance*> InstArray;
extern std::map<int, SNet*> NetArray;
extern std::map<int, SPin*> PinArray;
extern std::vector<STile*> TileArray;
extern SClockRegion ClockRegion_Info;
// Tile Map 的索引规则: X是列有150个，Y是行有300个，这样子会产生150*300=45000个位置
// 45000*30

// parameters
extern int bank_iteration;
extern int instance_iteration;
extern int indepSet_radius;
extern int indepSet_volume;
extern int indepSet_number;
extern bool dbinfo_enable;

void init_tiles();
void copy_instances();
void copy_nets();
void connection_setup();
int pindensity_setup();
bool in_pinset(int netID, int instID, SPin* pin);
bool out_pinset(int netID, int instID, SPin* pin);
void file_output(std::string filename);
void get_force(int iter);
void parse_config(std::ifstream &infile);
