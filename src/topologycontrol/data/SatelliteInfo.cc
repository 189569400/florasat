/*
 * SatelliteInfo.cc
 *
 * Created on: Feb 10, 2023
 *     Author: Robin Ohs
 */

#include "SatelliteInfo.h"

namespace flora
{
    namespace topologycontrol
    {
        std::string SatelliteInfo::to_string()
        {
            std::stringstream ss;
            ss << "{";
            ss << "\"satelliteId\": " << satelliteId << ",";
            ss << "\"satelliteModule\": " << satelliteModule << ",";
            ss << "\"noradModule\": " << noradModule << ",";
            ss << "\"groundStationId\": " << groundStationId << ",";
            ss << "}";
            return ss.str();
        }
    } // topologycontrol
} // flora
