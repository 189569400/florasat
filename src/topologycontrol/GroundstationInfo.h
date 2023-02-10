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
        int groundStationId;
        cModule *groundStation;
        GroundStationMobility *mobility;
        cModule* satellite;

        GroundstationInfo(int gsId, cModule *gs, GroundStationMobility* gsMobility) : groundStationId(gsId), groundStation(gs), mobility(gsMobility), satellite(nullptr){};

        std::string to_string();
    };
} // flora

#endif