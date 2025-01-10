#pragma once
#include "solverObject.h"
#include "checker_legacy/rsmt.h"
#include <cassert>

extern RecSteinerMinTree rsmt;

// void getMergedNonCritPinLocs(const SNet& net, std::vector<int>& xCoords, std::vector<int>& yCoords);

// int calculateWirelength(const SNet& net);

// int calculateInstanceWirelength(const SInstance& instance);

// int calculateWirelengthIncrease(const SInstance& instance_old, std::tuple<int, int, int> newLocation);

int calculate_WL_Increase(SInstance* inst_old_ptr, std::tuple<int, int, int> newLoc);
int calculate_bank_WL_Increase(STile* tile_old_ptr, bool oldbank, std::pair<int, int> newLoc);