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

ISLDirection DirectedRouting::RoutePacket(inet::Packet *pkt, cModule *callerSat) {
    auto frame = pkt->removeAtFront<RoutingHeader>();
    int destGroundstationId = frame->getDestinationGroundstation();
    pkt->insertAtFront(frame);

    int routingSatIndex = callerSat->getIndex();

    int destSatIndex = -1;

    // forward to which satellite? Here is a point to introduce shortest path etc.
    for (int number : topologyControl->getGroundstationInfo(destGroundstationId)->satellites) {
        destSatIndex = number;
        break;
    }
    if (destSatIndex == -1) {
        error("DestSatIndex was not set!");
    }

    // forward to which groundstation gate?
    if (routingSatIndex == destSatIndex) {
        flora::topologycontrol::GsSatConnection *gsSatConnection = topologyControl->getGroundstationSatConnection(destGroundstationId, destSatIndex);
        return ISLDirection(Direction::ISL_DOWNLINK, gsSatConnection->satGateIndex);
    }

    int myPlane = topologyControl->calculateSatellitePlane(routingSatIndex);
    int destPlane = topologyControl->calculateSatellitePlane(destSatIndex);
    bool isAscending = IsSatelliteAscending(callerSat);

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
        if (routingSatIndex < destSatIndex && HasConnection(callerSat, ISLDirection(Direction::ISL_UP, -1))) {
            return ISLDirection(Direction::ISL_UP, -1);
        } else if (routingSatIndex > destSatIndex && HasConnection(callerSat, ISLDirection(Direction::ISL_DOWN, -1))) {
            return ISLDirection(Direction::ISL_DOWN, -1);
        } else {
            error("Error in PacketHandlerDirected::handleMessage: choosen gate should be connected!");
        }
    }
    return ISLDirection(Direction::ISL_DOWN, -1);
}

}  // namespace routing
}  // namespace flora