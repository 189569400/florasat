/*
 * GroundstationInfo.h
 *
 * Created on: Feb 06, 2023
 *     Author: Robin Ohs
 */

#ifndef TOPOLOGYCONTROL_GROUNDSTATIONINFO_H
#define TOPOLOGYCONTROL_GROUNDSTATIONINFO_H

#include <omnetpp.h>
#include <set>
#include <sstream>
#include "mobility/GroundStationMobility.h"

using namespace omnetpp;

namespace flora
{
    namespace topologycontrol
    {
        struct GroundstationInfo
        {
            int groundStationId;
            cModule *groundStation;
            GroundStationMobility *mobility;
            std::set<int> satellites;
            GroundstationInfo(int gsId, cModule *gs, GroundStationMobility *gsMobility) : groundStationId(gsId),
                                                                                          groundStation(gs),
                                                                                          mobility(gsMobility){};
            std::string to_string();
        };
    } // topologycontrol
} // flora

#endif