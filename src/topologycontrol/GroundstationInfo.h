/*
 * GroundstationInfo.h
 *
 * Created on: Feb 06, 2023
 *     Author: Robin Ohs
 */

#ifndef TOPOLOGYCONTROL_GROUNDSTATIONINFO_H
#define TOPOLOGYCONTROL_GROUNDSTATIONINFO_H

#include <omnetpp.h>
#include <sstream>
#include "mobility/GroundStationMobility.h"

using namespace omnetpp;

namespace flora
{
    struct GroundstationInfo
    {
        cModule *groundStation;
        GroundStationMobility *mobility;
        std::vector<cModule *> satellites;

        GroundstationInfo(cModule *gs, GroundStationMobility* gsMobility) : groundStation(gs), mobility(gsMobility), satellites(){};

        std::string to_string();
    };
} // flora

#endif