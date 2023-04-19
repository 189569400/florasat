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
        topologyControl = check_and_cast<topologycontrol::TopologyControl *>(getSystemModule()->getSubmodule("topologyControl"));
        if (topologyControl == nullptr) {
            error("Error in DirectedRouting::initialize(): topologyControl is nullptr.");
        }
    }
}

bool RoutingBase::HasConnection(cModule *satellite, ISLDirection side) {
    if (satellite == nullptr)
        throw new cRuntimeError("RandomRouting::HasConnection(): satellite mullptr");

    switch (side.direction) {
        case ISL_UP:
            return satellite->gateHalf("up", omnetpp::cGate::Type::OUTPUT)->isConnectedOutside();
        case ISL_DOWN:
            return satellite->gateHalf("down", omnetpp::cGate::Type::OUTPUT)->isConnectedOutside();
        case ISL_LEFT:
            return satellite->gateHalf("left", omnetpp::cGate::Type::OUTPUT)->isConnectedOutside();
        case ISL_RIGHT:
            return satellite->gateHalf("right", omnetpp::cGate::Type::OUTPUT)->isConnectedOutside();
        case ISL_DOWNLINK:
            return satellite->gateHalf("groundLink", omnetpp::cGate::Type::OUTPUT, side.gateIndex)->isConnectedOutside();
        default:
            throw new cRuntimeError("HasConnection is not implemented for this side.");
    }
    return false;
}

int RoutingBase::GetGroundlinkIndex(int satelliteId, int groundstationId) {
    std::set<int> gsSatellites = GetConnectedSatellites(groundstationId);
    if (gsSatellites.find(satelliteId) != gsSatellites.end()) {
        return topologyControl->getGroundstationSatConnection(groundstationId, satelliteId)->satGateIndex;
    }
    return -1;
}

std::set<int> RoutingBase::GetConnectedSatellites(int groundStationId) {
    if (topologyControl == nullptr) error("Error in RoutingBase::GetGroundStationConnections(): topologyControl is nullptr. Did you call initialize on RoutingBase?");
    return topologyControl->getGroundstationInfo(groundStationId)->satellites;
}

bool RoutingBase::IsSatelliteAscending(cModule *satellite) {
    NoradA *noradA = dynamic_cast<NoradA *>(satellite->getSubmodule("NoradModule"));
    if (noradA == nullptr) {
        error("Error in RoutingBase::IsSatelliteAscending(): noradA module of loRaGW with index %d is nullptr. Make sure a module with name `NoradModule` exists.", satellite->getIndex());
    }
    return noradA->isAscending();
}

}  // namespace routing
}  // namespace flora