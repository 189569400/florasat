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

void DsprRouting::handleTopologyChange() {
    auto costMatrix = core::buildCostMatrix(topologyControl->getSatellites());
    for (size_t sId = 0; sId < topologyControl->getNumberOfSatellites(); sId++) {
        auto sat = topologyControl->getSatellite(sId);
        auto forwardingTable = check_and_cast<ForwardingTable *>(sat->getSubmodule("forwardingTable"));
        forwardingTable->clearRoutes();
        // run dijkstra to get shortest path to all satellites
        core::DijkstraResult result = core::runDijkstra(sId, costMatrix);
        for (size_t gsId = 0; gsId < topologyControl->getNumberOfGroundstations(); gsId++) {
            auto gs = topologyControl->getGroundstationInfo(gsId);
            // get lastSatellite with shortest distance + groundlink distance
            int shortestDistanceSat = -1;
            int shortestDistance = 999999999;
            for (auto pLastSat : gs->getSatellites()) {
                // add distance between sat and gs for selecting optimal last satellites
                double distance = result.distances[pLastSat] + gs->getDistance(*sat);
                if (shortestDistance > distance) {
                    shortestDistance = distance;
                    shortestDistanceSat = pLastSat;
                }
            }

            ASSERT(shortestDistanceSat != -1);

            // update forwarding table accordingly
            if (shortestDistanceSat == sId) {
                forwardingTable->setRoute(gsId, isldirection::GROUNDLINK);
            } else {
                int nextSatId = core::reconstructPath(sId, shortestDistanceSat, result.prev)[1];
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

Direction DsprRouting::routePacket(inet::Ptr<const RoutingHeader> rh, SatelliteRouting *callerSat) {
    auto forwardingTable = check_and_cast<ForwardingTable *>(callerSat->getSubmodule("forwardingTable"));
    return forwardingTable->getNextHop(rh->getDestinationGroundstation());
}

}  // namespace routing
}  // namespace flora
