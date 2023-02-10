/*
 * GroundstationInfo.cc
 *
 * Created on: Feb 06, 2023
 *     Author: Robin Ohs
 */

#include "GroundstationInfo.h"

namespace flora
{
    std::string GroundstationInfo::to_string()
    {
        std::stringstream ss;
        ss << groundStation << ": {";
        ss << satellite;
        ss << "}";
        return ss.str();
    }
} // flora
