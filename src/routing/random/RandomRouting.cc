/*
 * RandomRouting.cc
 *
 * Created on: Feb 04, 2023
 *     Author: Robin Ohs
 */

#include "RandomRouting.h"

namespace flora
{
    Define_Module(RandomRouting);

    ISLDirection RandomRouting::RoutePacket(inet::Packet *pkt, cModule *callerSat)
    {
        int gate = intuniform(0, 3);

        if (gate == 0 && HasConnection(callerSat, ISLDirection::ISL_DOWN)) {
            return ISLDirection::ISL_DOWN;
        } else if (gate == 1 && HasConnection(callerSat, ISLDirection::ISL_UP)) {
            return ISLDirection::ISL_UP;
        } else if (gate == 2 && HasConnection(callerSat, ISLDirection::ISL_LEFT)) {
            return ISLDirection::ISL_LEFT;
        } else if (gate == 3 && HasConnection(callerSat, ISLDirection::ISL_RIGHT)) {
            return ISLDirection::ISL_RIGHT;
        }
        return RoutePacket(pkt, callerSat);
    }

    bool RandomRouting::HasConnection(cModule* satellite, ISLDirection side)
    {
        if (satellite == nullptr)
            error("RoutingBase::HasConnection(): satellite mullptr");
        switch (side)
        {
        case ISL_UP:
            return satellite->gateHalf("up", cGate::Type::OUTPUT)->isConnectedOutside();
        case ISL_DOWN:
            return satellite->gateHalf("down", cGate::Type::OUTPUT)->isConnectedOutside();
        case ISL_LEFT:
            return satellite->gateHalf("left", cGate::Type::OUTPUT)->isConnectedOutside();
        case ISL_RIGHT:
            return satellite->gateHalf("right", cGate::Type::OUTPUT)->isConnectedOutside();
        case ISL_DOWNLINK:
            return satellite->gateHalf("groundLink", cGate::Type::OUTPUT)->isConnectedOutside();
        default:
            error("HasConnection is not implemented for this side.");
        }
        return false;
    }
}