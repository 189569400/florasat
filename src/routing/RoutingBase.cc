/*
 * RoutingBase.cc
 *
 * Created on: Feb 04, 2023
 *     Author: Robin Ohs
 */

#include "RoutingBase.h"

namespace flora {
namespace routing {

void RoutingBase::initialize(int stage) {
    if (stage == inet::INITSTAGE_LOCAL) {
        topologyControl = check_and_cast<topologycontrol::TopologyControlBase *>(getSystemModule()->getSubmodule("topologyControl"));
        if (topologyControl == nullptr) {
            error("Error in DirectedRouting::initialize(): topologyControl is nullptr.");
        }
    }
}

std::pair<int, int> RoutingBase::calculateFirstAndLastSatellite(int srcGs, int dstGs) {
    auto srcGsInfo = topologyControl->getGroundstationInfo(srcGs);
    auto dstGsInfo = topologyControl->getGroundstationInfo(dstGs);
    if (!srcGsInfo->isConnectedToAnySat()) {
        error("Error in RoutingBase::calculateFirstAndLastSatellite: srcGsInfo has no satellite connections");
    }
    if (!dstGsInfo->isConnectedToAnySat()) {
        error("Error in RoutingBase::calculateFirstAndLastSatellite: dstGsInfo has no satellite connections");
    }

    int firstSat = -1;
    int lastSat = -1;
    double minDistance = -1;

    EV_DEBUG << "<><><><><><><><>" << endl;
    EV_DEBUG << "Calculate first and last sat. Distances " << srcGs << "->" << dstGs << ":" << endl;

    auto costMatrix = routing::core::buildCostMatrix(topologyControl->getSatellites());

    for (auto pFirstSat : srcGsInfo->getSatellites()) {
        routing::core::DijkstraResult result = routing::core::runDijkstra(pFirstSat, costMatrix);
        for (auto pLastSat : dstGsInfo->getSatellites()) {
            auto distance = result.distances[pLastSat];
            auto glDistance = dstGsInfo->getDistance(*topologyControl->getSatellite(pLastSat));

#ifndef NDEBUG
            auto path = routing::core::reconstructPath(pFirstSat, pLastSat, result.prev);
            EV_DEBUG << "Distance(" << pFirstSat << "," << pLastSat << ") = " << distance << " + " << glDistance << "; Route: [" << flora::core::utils::vector::toString(path.begin(), path.end()) << "]" << endl;
#endif

            distance = distance + glDistance;

            if (minDistance == -1 || distance < minDistance) {
                firstSat = pFirstSat;
                lastSat = pLastSat;
                minDistance = distance;
            }
        }
    }

    EV_DEBUG << "Shortest path (" << srcGs << "->" << dstGs << ") found with firstSat (" << firstSat << ") and lastSat (" << lastSat << ")"
             << "| Distance: " << minDistance << "km" << endl;
    EV_DEBUG << "<><><><><><><><>" << endl;

    ASSERT(firstSat != -1);
    ASSERT(lastSat != -1);
    ASSERT(minDistance != -1);

    return std::make_pair(firstSat, lastSat);
}

int RoutingBase::getGroundlinkIndex(int satelliteId, int groundstationId) {
    std::set<int> gsSatellites = getConnectedSatellites(groundstationId);
    if (gsSatellites.find(satelliteId) != gsSatellites.end()) {
        return topologyControl->getGroundstationSatConnection(groundstationId, satelliteId).satGateIndex;
    }
    return -1;
}

std::set<int> const &RoutingBase::getConnectedSatellites(int groundStationId) const {
    if (topologyControl == nullptr) error("Error in RoutingBase::GetGroundStationConnections(): topologyControl is nullptr. Did you call initialize on RoutingBase?");
    return topologyControl->getGroundstationInfo(groundStationId)->getSatellites();
}

}  // namespace routing
}  // namespace flora