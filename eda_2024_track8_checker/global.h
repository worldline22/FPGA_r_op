#pragma once

#include <map>
#include <string>
#include "object.h"
#include "lib.h"
#include "arch.h"
#include "rsmt.h"

// global variables
extern std::map<std::string, Lib*> glbLibMap;
extern std::map<int, Instance*> glbInstMap;
extern std::map<int, Net*> glbNetMap;
extern Arch chip;
extern RecSteinerMinTree rsmt;
extern std::string lineBreaker;