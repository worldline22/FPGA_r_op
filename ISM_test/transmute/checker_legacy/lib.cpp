#include <iostream>
#include <fstream>
#include <sstream>
#include "lib.h"
#include "global.h"

bool readAndCreateLib(std::string libFilename) {
    if (glbLibMap.empty() == false) {
        std::cout << "Library map is not empty" << std::endl;
        return false;
    }

    // read lib file
    std::ifstream libFile(libFilename);
    if (!libFile.is_open()) {
        std::cout << "Failed to open file: " << libFilename << std::endl;
        return false;
    }

    std::string line;
    std::vector<std::string> pinLines;
    unsigned int numErr = 0;
    bool isInsideCell = false;
    std::string cellNameStr;
    while (std::getline(libFile, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }

        if (line.substr(0, 4) == "CELL") {
            isInsideCell = true;
            cellNameStr = line.substr(4);
        }
        if (isInsideCell) {
            pinLines.push_back(line);
        }

        if (line.substr(0, 8) == "END_CELL") {
            isInsideCell = false;
            
            // process lib 
            std::istringstream iss(cellNameStr);
            std::vector<std::string> tokens;
            std::string token;
            while (iss >> token) {
                tokens.push_back(token);
            }

            cellNameStr = tokens[0];

            // check if lib already exists in map
            if (glbLibMap.find(cellNameStr) != glbLibMap.end()) {
                std::cout << "Library already exists: " << cellNameStr << std::endl;
                numErr++;
                continue;
            } else {
                // create new lib
                Lib* newLib = new Lib(cellNameStr);

                // create pin of this lib
                int numInPin = 0;
                int numOutPin = 0;
                for (auto pin : pinLines) {
                    std::istringstream pinss(pin);
                    std::vector<std::string> pinTokens;
                    std::string pinToken;
                    while (pinss >> pinToken) {
                        pinTokens.push_back(pinToken);
                    }
                    if ( pinTokens.size() != 3 && pinTokens.size() != 4) {
                        continue;
                    }
                    if (pinTokens[2] == "INPUT") {
                        numInPin++;
                    } else if (pinTokens[2] == "OUTPUT") {
                        numOutPin++;
                    } else {
                        std::cout << "Invalid pin type: " << pinTokens[2] << std::endl;
                        numErr++;
                        continue;
                    }
                }
                newLib->setNumInputs(numInPin);
                newLib->setNumOutputs(numOutPin);  

                for (auto pin : pinLines) {
                    std::istringstream pinss(pin);
                    std::vector<std::string> pinTokens;
                    std::string pinToken;
                    while (pinss >> pinToken) {
                        pinTokens.push_back(pinToken);
                    }
                    if ( pinTokens.size() != 3 && pinTokens.size() != 4) {
                        continue;
                    }

                    PinProp prop = PIN_PROP_NONE;
                    if (pinTokens.size() == 4) {
                        if (pinTokens[3] == "CTRL") {
                            prop = PIN_PROP_CE;
                        } else if (pinTokens[3] == "CLOCK") {
                            prop = PIN_PROP_CLOCK;
                        } else if (pinTokens[3] == "RESET") {
                            prop = PIN_PROP_RESET;
                        } else {
                            std::cout << "Invalid pin property: " << pinTokens[3] << std::endl;
                            numErr++;
                            continue;
                        } 
                    }                    

                    size_t underscorePos = pinTokens[1].find('_');
                    std::string subStr = pinTokens[1].substr(underscorePos + 1);
                    int pinIdx = std::stoi(subStr);
                    if (pinTokens[2] == "INPUT") {
                        newLib->setInput(pinIdx, pinTokens[1], prop);
                    } else if (pinTokens[2] == "OUTPUT") {
                        newLib->setOutput(pinIdx, pinTokens[1], prop);
                    } else {
                        std::cout << "Invalid pin type: " << pinTokens[2] << std::endl;
                        numErr++;
                        continue;
                    }
                }
                // add lib to map
                glbLibMap[cellNameStr] = newLib;
                pinLines.clear();
            }
        }
    }

    if (numErr > 0) {
        return false;
    } else {
        return true;
    }

}