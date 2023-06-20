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

void DsprRouting::initialize(int stage) {
    RoutingBase::initialize(stage);
}

void DsprRouting::handleTopologyChange() {
    Enter_Method("handleTopologyChange");
    // generate cost matrix for topology
    int constellationSize = topologyControl->getNumberOfSatellites();
    auto costMatrix = core::dspa::buildShortestPathCostMatrix(constellationSize, topologyControl->getSatellites());

    for (size_t sId = 0; sId < constellationSize; sId++) {
        auto sat = topologyControl->getSatellite(sId);
        auto forwardingTable = static_cast<ForwardingTable *>(sat->getSubmodule("forwardingTable"));
        forwardingTable->clearRoutes();
        // run dijkstra to get shortest path to all satellites
        core::dspa::DijkstraResultFull dijkstraResultFull = core::dspa::runDijkstraFull(costMatrix, sId);
        for (size_t gsId = 0; gsId < topologyControl->getNumberOfGroundstations(); gsId++) {
            auto gs = topologyControl->getGroundstationInfo(gsId);

            // get lastSatellite with shortest distance
            int nearestSat = core::dspa::getNearestId(dijkstraResultFull, gs->getSatellites());

            // update forwarding table accordingly
            if (nearestSat == sId) {
                forwardingTable->setRoute(gsId, isldirection::GROUNDLINK);
            } else {
                int nextSatId = core::dspa::reconstructPath(dijkstraResultFull, nearestSat)[1];
                if (sat->hasLeftSat() && sat->getLeftSatId() == nextSatId) {
                    forwardingTable->setRoute(gsId, isldirection::LEFT);
                } else if (sat->hasUpSat() && sat->getUpSatId() == nextSatId) {
                    forwardingTable->setRoute(gsId, isldirection::UP);
                } else if (sat->hasRightSat() && sat->getRightSatId() == nextSatId) {
                    forwardingTable->setRoute(gsId, isldirection::RIGHT);
                } else if (sat->hasDownSat() && sat->getDownSatId() == nextSatId) {
                    forwardingTable->setRoute(gsId, isldirection::DOWN);
                } else {
                    error("Path not found.");
                }
            }
        }
#ifndef NDEBUG
        forwardingTable->printForwardingTable();
#endif
    }
}

ISLDirection DsprRouting::routePacket(inet::Ptr<const RoutingHeader> rh, SatelliteRouting *callerSat) {
    Enter_Method("routePacket");
    auto forwardingTable = static_cast<ForwardingTable *>(callerSat->getSubmodule("forwardingTable"));
    return forwardingTable->getNextHop(rh->getDstGs());
}

}  // namespace routing
}  // namespace flora
