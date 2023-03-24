/*
 * GroundstationInfo.h
 *
 * Created on: Feb 06, 2023
 *     Author: Robin Ohs
 */

#ifndef __FLORA_TOPOLOGYCONTROL_GROUNDSTATIONINFO_H_
#define __FLORA_TOPOLOGYCONTROL_GROUNDSTATIONINFO_H_

#include <omnetpp.h>

#include <set>
#include <sstream>

#include "mobility/GroundStationMobility.h"

using namespace omnetpp;

namespace flora {
namespace topologycontrol {

struct GroundstationInfo {
    int groundStationId;
    cModule *groundStation;
    GroundStationMobility *mobility;
    std::set<int> satellites;
    GroundstationInfo(int gsId, cModule *gs, GroundStationMobility *gsMobility) : groundStationId(gsId),
                                                                                  groundStation(gs),
                                                                                  mobility(gsMobility){};
    std::string to_string();
};

}  // namespace topologycontrol
}  // namespace flora

#endif  // __FLORA_TOPOLOGYCONTROL_GROUNDSTATIONINFO_H_