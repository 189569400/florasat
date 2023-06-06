/*
 * RandomRouting.cc
 *
 * Created on: Feb 04, 2023
 *     Author: Robin Ohs
 */

#include "RandomRouting.h"

namespace flora {
namespace routing {

Define_Module(RandomRouting);

ISLDirection RandomRouting::routePacket(inet::Ptr<const flora::RoutingHeader> rh, SatelliteRouting *callerSat) {
    Enter_Method("routePacket");
    int dstGs = rh->getDstGs();
    int callerSatIndex = callerSat->getIndex();

    // check if connected to destination groundstation
    int groundlinkIndex = RoutingBase::getGroundlinkIndex(callerSatIndex, dstGs);
    if (groundlinkIndex != -1) {
        return ISLDirection::GROUNDLINK;
    }

    // if not connected to destination find random
    while (true) {
        int gate = intrand(4);
        if (gate == 0 && callerSat->hasLeftSat()) {
            return ISLDirection::LEFT;
        } else if (gate == 1 && callerSat->hasUpSat()) {
            return ISLDirection::UP;
        } else if (gate == 2 && callerSat->hasRightSat()) {
            return ISLDirection::RIGHT;
        } else if (gate == 3 && callerSat->hasDownSat()) {
            return ISLDirection::DOWN;
        } else {
            error("No route found.");
        }
    }
}

}  // namespace routing
}  // namespace flora