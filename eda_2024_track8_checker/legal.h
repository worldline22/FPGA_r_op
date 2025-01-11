#pragma once

#include <iostream>
#include <map>
#include "object.h"
#include "arch.h"

bool legalCheck();  // check tile type and capacity 

bool checkTypeAndCapacity(bool isBaseline);
bool checkControlSet(bool isBaseline);
bool checkClockRegion(bool isBaseline);

void reportClockRegion(const int col, const int row);