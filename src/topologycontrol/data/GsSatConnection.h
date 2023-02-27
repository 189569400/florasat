/*
 * GsSatConnection.h
 *
 * Created on: Feb 23, 2023
 *     Author: Robin Ohs
 */

#ifndef TOPOLOGYCONTROL_GSSATCONNECTIONINFO_H
#define TOPOLOGYCONTROL_GSSATCONNECTIONINFO_H

#include <omnetpp.h>
#include <sstream>
#include "GroundstationInfo.h"
#include "SatelliteInfo.h"

using namespace omnetpp;

namespace flora
{
    namespace topologycontrol
    {
        struct GsSatConnection
        {
            GroundstationInfo *gsInfo;
            int gsGateIndex = -1;
            SatelliteInfo *satInfo;
            int satGateIndex = -1;

            GsSatConnection(GroundstationInfo *gInfo, SatelliteInfo *sInfo, int gGateIndex, int sGateIndex)
                : gsInfo(gInfo),
                  gsGateIndex(gGateIndex),
                  satInfo(sInfo),
                  satGateIndex(sGateIndex){};
            std::string to_string();
        };
    } // topologycontrol
} // flora

#endif