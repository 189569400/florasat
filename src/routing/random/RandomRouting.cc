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

Direction RandomRouting::routePacket(inet::Ptr<const flora::RoutingHeader> rh, SatelliteRouting *callerSat) {
    Enter_Method("routePacket", rh, callerSat);
    int dstGs = rh->getDestinationGroundstation();
    int callerSatIndex = callerSat->getIndex();

    // check if connected to destination groundstation
    int groundlinkIndex = RoutingBase::getGroundlinkIndex(callerSatIndex, dstGs);
    if (groundlinkIndex != -1) {
        return Direction::ISL_DOWNLINK;
    }

    // if not connected to destination find random
    int gate = intrand(4);
    if (gate == 0 && callerSat->hasLeftSat()) {
        return Direction::ISL_LEFT;
    } else if (gate == 1 && callerSat->hasUpSat()) {
        return Direction::ISL_UP;
    } else if (gate == 2 && callerSat->hasRightSat()) {
        return Direction::ISL_RIGHT;
    } else if (gate == 3 && callerSat->hasDownSat()) {
        return Direction::ISL_DOWN;
    }
    return routePacket(rh, callerSat);
}

}  // namespace routing
}  // namespace flora