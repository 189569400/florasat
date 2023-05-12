/*
 * DirectedRouting.cc
 *
 * Created on: Feb 04, 2023
 *     Author: Robin Ohs
 */

#include "DirectedRouting.h"

namespace flora {
namespace routing {

Define_Module(DirectedRouting);

ISLDirection DirectedRouting::routePacket(inet::Ptr<RoutingHeader> frame, cModule *callerSat) {
    int routerSatIndex = callerSat->getIndex();
    int destSatIndex = frame->getLastSatellite();

    // this is the last satellite
    if (routerSatIndex == destSatIndex) {
        int destGroundstationId = frame->getDestinationGroundstation();
        flora::topologycontrol::GsSatConnection gsSatConnection = topologyControl->getGroundstationSatConnection(destGroundstationId, destSatIndex);
        return ISLDirection(Direction::ISL_DOWNLINK, gsSatConnection.satGateIndex);
    }

    topologycontrol::SatelliteInfo routerSat = topologyControl->getSatelliteInfo(routerSatIndex);
    int myPlane = routerSat.getPlane();
    bool isAscending = routerSat.isAscending();
    int destPlane = topologyControl->getSatelliteInfo(destSatIndex).getPlane();

    // destination is on same plane
    if (myPlane < destPlane) {
        if (isAscending) {
            if (HasConnection(callerSat, ISLDirection(Direction::ISL_RIGHT, -1))) {
                return ISLDirection(Direction::ISL_RIGHT, -1);
            } else {
                return ISLDirection(Direction::ISL_DOWN, -1);
            }
        } else {
            if (HasConnection(callerSat, ISLDirection(Direction::ISL_LEFT, -1))) {
                return ISLDirection(Direction::ISL_LEFT, -1);
            } else {
                return ISLDirection(Direction::ISL_UP, -1);
            }
        }
    } else if (myPlane > destPlane) {
        if (isAscending) {
            if (HasConnection(callerSat, ISLDirection(Direction::ISL_LEFT, -1))) {
                return ISLDirection(Direction::ISL_LEFT, -1);
            } else {
                return ISLDirection(Direction::ISL_DOWN, -1);
            }
        } else {
            if (HasConnection(callerSat, ISLDirection(Direction::ISL_RIGHT, -1))) {
                return ISLDirection(Direction::ISL_RIGHT, -1);
            } else {
                return ISLDirection(Direction::ISL_UP, -1);
            }
        }
    } else if (myPlane == destPlane) {
        if (routerSatIndex < destSatIndex && HasConnection(callerSat, ISLDirection(Direction::ISL_UP, -1))) {
            return ISLDirection(Direction::ISL_UP, -1);
        } else if (routerSatIndex > destSatIndex && HasConnection(callerSat, ISLDirection(Direction::ISL_DOWN, -1))) {
            return ISLDirection(Direction::ISL_DOWN, -1);
        } else {
            error("Error in PacketHandlerDirected::handleMessage: choosen gate should be connected!");
        }
    }
    return ISLDirection(Direction::ISL_DOWN, -1);
}

}  // namespace routing
}  // namespace flora