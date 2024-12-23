#include "solverObject.h"
#include "global.h"
#include "rsmt.h"

int calculateWirelength(const SNet& net);

int calculateInstanceWirelength(const SInstance& instance);

int calculateWirelengthIncrease(const SInstance& instance_old, std::tuple<int, int, int> newLocation);