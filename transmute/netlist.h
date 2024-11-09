#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>


bool readInputNodes(const std::string& fileName);
bool readInputNets(const std::string& fileName);
bool readOutputNetlist(const std::string& fileName);
bool readInputTiming(const std::string& fileName);

bool reportDesignStatistics();
