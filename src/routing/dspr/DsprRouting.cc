/*
 * DsprRouting.cc
 *
 * Created on: Feb 04, 2023
 *     Author: Robin Ohs
 */

#include "DsprRouting.h"

namespace flora {
namespace routing {

Define_Module(DsprRouting);

void DsprRouting::initRouting(inet::Packet *pkt, cModule *callerSat) {
    auto frame = pkt->removeAtFront<RoutingHeader>();
    int src = frame->getFirstSatellite();
    int dst = frame->getLastSatellite();
    core::DijkstraResult result = core::dijkstraEarlyAbort(src, dst, topologyControl->getSatelliteInfos());
    std::vector<int> path = core::reconstructPath(src, dst, result.prev);
    for (auto t : path) {
        EV << t << endl;
        frame->appendPath(t);
    }
    pkt->insertAtFront(frame);
}

ISLDirection DsprRouting::routePacket(inet::Packet *pkt, cModule *callerSat) {
    auto frame = pkt->removeAtFront<RoutingHeader>();
    int thisSatId = callerSat->getIndex();
    int destGroundstationId = frame->getDestinationGroundstation();
    int lastSatellite = frame->getLastSatellite();

    // last satellite reached
    if (lastSatellite == thisSatId) {
        pkt->insertAtFront(frame);
        flora::topologycontrol::GsSatConnection gsSatConnection = topologyControl->getGroundstationSatConnection(destGroundstationId, lastSatellite);
        return ISLDirection(Direction::ISL_DOWNLINK, gsSatConnection.satGateIndex);
    }

    if (frame->getPathArraySize() == 0) {
        throw new cRuntimeError("Error in DsprRouting::routePacket: Next hop was not found.");
    }
    frame->erasePath(0);
    int nextSat = frame->getPath(0);
    pkt->insertAtFront(frame);

    auto thisSatInfo = topologyControl->getSatelliteInfo(thisSatId);
    if (thisSatInfo.hasLeftSat() && thisSatInfo.getLeftSat() == nextSat) {
        return ISLDirection(Direction::ISL_LEFT, -1);
    } else if (thisSatInfo.hasUpSat() && thisSatInfo.getUpSat() == nextSat) {
        return ISLDirection(Direction::ISL_UP, -1);
    } else if (thisSatInfo.hasRightSat() && thisSatInfo.getRightSat() == nextSat) {
        return ISLDirection(Direction::ISL_RIGHT, -1);
    } else if (thisSatInfo.hasDownSat() && thisSatInfo.getDownSat() == nextSat) {
        return ISLDirection(Direction::ISL_DOWN, -1);
    }
    throw new cRuntimeError("Error in DsprRouting::routePacket: Next routing direction was not found in satinfo.");
}

}  // namespace routing
}  // namespace flora