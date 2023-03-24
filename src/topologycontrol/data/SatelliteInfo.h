/*
 * SatelliteInfo.h
 *
 * Created on: Feb 10, 2023
 *     Author: Robin Ohs
 */

#ifndef __FLORA_TOPOLOGYCONTROL_SATELLITEINFO_H_
#define __FLORA_TOPOLOGYCONTROL_SATELLITEINFO_H_

#include <omnetpp.h>

#include <sstream>

#include "mobility/NoradA.h"

using namespace omnetpp;

namespace flora {
namespace topologycontrol {

struct SatelliteInfo {
    int satelliteId;
    cModule *satelliteModule;
    NoradA *noradModule;

    int leftSatellite = -1;
    int rightSatellite = -1;
    int upSatellite = -1;
    int downSatellite = -1;

    SatelliteInfo(int satId, cModule *satModule, NoradA *norad) : satelliteId(satId),
                                                                  satelliteModule(satModule),
                                                                  noradModule(norad){};

    std::string to_string();
};

}  // namespace topologycontrol
}  // namespace flora

#endif  // __FLORA_TOPOLOGYCONTROL_SATELLITEINFO_H_