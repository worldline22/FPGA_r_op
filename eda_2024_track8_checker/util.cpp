#include <iostream>
#include "util.h"

std::string unifyModelType(std::string inputType) {

    std::string subType = inputType.substr(0, 3);
    std::string trimmedType = inputType;
    if (subType == "LUT") {
        trimmedType = subType;
    } 
    return trimmedType;
}