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

ISLDirection RandomRouting::RoutePacket(inet::Packet *pkt, cModule *callerSat) {
    auto frame = pkt->removeAtFront<RoutingHeader>();
    int destinationGroundStation = frame->getDestinationGroundstation();
    pkt->insertAtFront(frame);
    int callerSatIndex = callerSat->getIndex();

    // check if connected to destination groundstation
    int groundlinkIndex = RoutingBase::GetGroundlinkIndex(callerSatIndex, destinationGroundStation);
    if (groundlinkIndex != -1) {
        if (RoutingBase::HasConnection(callerSat, ISLDirection(Direction::ISL_DOWNLINK, groundlinkIndex))) {
            return ISLDirection(Direction::ISL_DOWNLINK, groundlinkIndex);
        } else {
            error("Error in RandomRouting::RoutePacket: There should be a connection between groundstation and satellite but is not connected.");
        }
    }

    // if not connected to destination find random
    int gate = intrand(4);
    if (gate == 0 && RoutingBase::HasConnection(callerSat, ISLDirection(Direction::ISL_DOWN, -1))) {
        return ISLDirection(Direction::ISL_DOWN, -1);
    } else if (gate == 1 && RoutingBase::HasConnection(callerSat, ISLDirection(Direction::ISL_UP, -1))) {
        return ISLDirection(Direction::ISL_UP, -1);
    } else if (gate == 2 && RoutingBase::HasConnection(callerSat, ISLDirection(Direction::ISL_LEFT, -1))) {
        return ISLDirection(Direction::ISL_LEFT, -1);
    } else if (gate == 3 && RoutingBase::HasConnection(callerSat, ISLDirection(Direction::ISL_RIGHT, -1))) {
        return ISLDirection(Direction::ISL_RIGHT, -1);
    }
    return RoutePacket(pkt, callerSat);
}

}  // namespace routing
}  // namespace flora