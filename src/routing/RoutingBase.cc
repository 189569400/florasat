/*
 * RoutingBase.cc
 *
 * Created on: Feb 04, 2023
 *     Author: Robin Ohs
 */

#include "RoutingBase.h"

namespace flora {
namespace routing {

bool RoutingBase::HasConnection(omnetpp::cModule *satellite, ISLDirection side) {
    if (satellite == nullptr)
        throw new omnetpp::cRuntimeError("RandomRouting::HasConnection(): satellite mullptr");
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
            throw new omnetpp::cRuntimeError("HasConnection is not implemented for this side.");
    }
    return false;
}

}  // namespace routing
}  // namespace flora