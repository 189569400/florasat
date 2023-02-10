/*
 * SatelliteInfo.h
 *
 * Created on: Feb 10, 2023
 *     Author: Robin Ohs
 */

#ifndef TOPOLOGYCONTROL_SATELLITEINFO_H
#define TOPOLOGYCONTROL_SATELLITEINFO_H

#include <omnetpp.h>
#include <sstream>
#include "mobility/NoradA.h"

using namespace omnetpp;

namespace flora
{
    namespace topologycontrol
    {
        struct SatelliteInfo
        {
            int satelliteId;
            cModule *satelliteModule;
            NoradA *noradModule;
            int groundStationId = -1;

            SatelliteInfo(int satId, cModule *satModule, NoradA *norad) : satelliteId(satId),
                                                                          satelliteModule(satModule),
                                                                          noradModule(norad){};

            std::string to_string();
        };
    } // topologycontrol
} // flora

#endif