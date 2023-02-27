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
            ss << "\"up\": " << upSatellite << ",";
            ss << "\"down\": " << downSatellite << ",";
            ss << "\"left\": " << leftSatellite << ",";
            ss << "\"right\": " << rightSatellite << ",";
            ss << "}";
            return ss.str();
        }
    } // topologycontrol
} // flora
