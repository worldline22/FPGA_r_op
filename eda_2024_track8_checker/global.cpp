#include "global.h"

std::map<std::string, Lib*> glbLibMap;
std::map<int, Instance*> glbInstMap;
std::map<int, Net*> glbNetMap;
Arch chip;
RecSteinerMinTree rsmt;
std::string lineBreaker = "------------------------------------------";
