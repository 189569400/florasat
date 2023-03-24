/*
 * RandomRouting.cc
 *
 * Created on: Feb 04, 2023
 *     Author: Robin Ohs
 */

#include "RandomRouting.h"

namespace flora {

Define_Module(RandomRouting);

ISLDirection RandomRouting::RoutePacket(inet::Packet *pkt, cModule *callerSat) {
    int gate = intuniform(0, 3);

    if (gate == 0 && HasConnection(callerSat, ISLDirection(Direction::ISL_DOWN, -1))) {
        return ISLDirection(Direction::ISL_DOWN, -1);
    } else if (gate == 1 && HasConnection(callerSat, ISLDirection(Direction::ISL_UP, -1))) {
        return ISLDirection(Direction::ISL_UP, -1);
    } else if (gate == 2 && HasConnection(callerSat, ISLDirection(Direction::ISL_LEFT, -1))) {
        return ISLDirection(Direction::ISL_LEFT, -1);
    } else if (gate == 3 && HasConnection(callerSat, ISLDirection(Direction::ISL_RIGHT, -1))) {
        return ISLDirection(Direction::ISL_RIGHT, -1);
    }
    return RoutePacket(pkt, callerSat);
}

bool RandomRouting::HasConnection(cModule *satellite, ISLDirection side) {
    if (satellite == nullptr)
        error("RandomRouting::HasConnection(): satellite mullptr");
    switch (side.direction) {
        case ISL_UP:
            return satellite->gateHalf("up", cGate::Type::OUTPUT)->isConnectedOutside();
        case ISL_DOWN:
            return satellite->gateHalf("down", cGate::Type::OUTPUT)->isConnectedOutside();
        case ISL_LEFT:
            return satellite->gateHalf("left", cGate::Type::OUTPUT)->isConnectedOutside();
        case ISL_RIGHT:
            return satellite->gateHalf("right", cGate::Type::OUTPUT)->isConnectedOutside();
        case ISL_DOWNLINK:
            return satellite->gateHalf("groundLink", cGate::Type::OUTPUT, side.gateIndex)->isConnectedOutside();
        default:
            error("HasConnection is not implemented for this side.");
    }
    return false;
}

}  // namespace flora