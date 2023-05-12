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
    int srcSat = frame->getFirstSatellite();
    int dstSat = frame->getLastSatellite();
#ifndef NDEBUG
    EV << "<><><><><><><><>" << endl;
    EV << "Initialize routing from " << srcSat << "->" << dstSat << endl;
#endif

    core::DijkstraResult result = core::dijkstraEarlyAbort(srcSat, dstSat, topologyControl->getSatelliteInfos());
    std::vector<int> path = core::reconstructPath(srcSat, dstSat, result.prev);
    for (auto t : path) {
        frame->appendPath(t);
    }
    pkt->insertAtFront(frame);
#ifndef NDEBUG
    EV << "Path: [" << flora::core::utils::vector::toString(path.begin(), path.end()) << "]" << endl;
    EV << "<><><><><><><><>" << endl;
#endif
}

ISLDirection DsprRouting::routePacket(inet::Ptr<RoutingHeader> frame, cModule *callerSat) {
    int thisSatId = callerSat->getIndex();
    int lastSatId = frame->getLastSatellite();

    // last satellite reached
    if (thisSatId == lastSatId) {
        flora::topologycontrol::GsSatConnection gsSatConnection = topologyControl->getGroundstationSatConnection(frame->getDestinationGroundstation(), lastSatId);
        return ISLDirection(Direction::ISL_DOWNLINK, gsSatConnection.satGateIndex);
    }

    if (frame->getPathArraySize() == 0) throw new cRuntimeError("Error in DsprRouting::routePacket: Next hop was not found.");
    frame->erasePath(0);
    int nextSatId = frame->getPath(0);

#ifndef NDEBUG
    std::vector<int> remRoute;
    for (size_t i = 1; i < frame->getPathArraySize(); i++) {
        remRoute.emplace_back(frame->getPath(i));
    }
    EV << "(Sat " << thisSatId << ") Next hop: " << nextSatId << "; Remaining Path: [" << flora::core::utils::vector::toString(remRoute.begin(), remRoute.end()) << "]" << endl;
#endif

    auto thisSatInfo = topologyControl->getSatelliteInfo(thisSatId);
    if (thisSatInfo.hasLeftSat() && thisSatInfo.getLeftSat() == nextSatId) {
        return ISLDirection(Direction::ISL_LEFT, -1);
    } else if (thisSatInfo.hasUpSat() && thisSatInfo.getUpSat() == nextSatId) {
        return ISLDirection(Direction::ISL_UP, -1);
    } else if (thisSatInfo.hasRightSat() && thisSatInfo.getRightSat() == nextSatId) {
        return ISLDirection(Direction::ISL_RIGHT, -1);
    } else if (thisSatInfo.hasDownSat() && thisSatInfo.getDownSat() == nextSatId) {
        return ISLDirection(Direction::ISL_DOWN, -1);
    }
    throw new cRuntimeError("Error in DsprRouting::routePacket: Next routing direction was not found in satinfo.");
}

}  // namespace routing
}  // namespace flora