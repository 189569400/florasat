/*
 * GroundstationInfo.cc
 *
 * Created on: Feb 06, 2023
 *     Author: Robin Ohs
 */

#include "GroundstationInfo.h"

namespace flora
{
    namespace topologycontrol
    {
        std::string GroundstationInfo::to_string()
        {
            std::stringstream ss;
            ss << "{";
            ss << "\"groundStationId\": " << groundStationId << ",";
            ss << "\"groundStation\": " << groundStation << ",";
            ss << "\"satelliteId\": " << satelliteId << ",";
            ss << "}";
            return ss.str();
        }
    } // topologycontrol
} // flora
