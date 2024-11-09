#pragma once
#include <list>
#include <vector>
#include <string>
#include <map>
#include <tuple>
#include <..\checker_legacy\object.h>

struct SSlot
{
    std::list<int> baseline_InstIDs;
    std::list<int> current_InstIDs;
};

struct STile
{
    int X; // col
    int Y; // row
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

