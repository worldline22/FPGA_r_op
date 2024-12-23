#pragma once
#include "solverObject.h"
#include "checker_legacy/rsmt.h"

extern RecSteinerMinTree rsmt;

void getMergedNonCritPinLocs(const SNet& net, std::vector<int>& xCoords, std::vector<int>& yCoords);

int calculateWirelength(const SNet& net);

int calculateInstanceWirelength(const SInstance& instance);

int calculateWirelengthIncrease(const SInstance& instance_old, std::tuple<int, int, int> newLocation);