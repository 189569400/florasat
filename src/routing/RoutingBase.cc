/*
 * RoutingBase.cc
 *
 * Created on: Feb 04, 2023
 *     Author: Robin Ohs
 */

#include "RoutingBase.h"

namespace flora
{
    Define_Module(RoutingBase);

    ISLDirection RoutingBase::RoutePacket(inet::Packet *pkt, cModule *callerSat)
    {
        return ISLDirection(Direction::ISL_DOWN, -1);
    }

    bool RoutingBase::HasConnection(cModule* satellite, ISLDirection side)
    {
        if (satellite == nullptr)
            error("RandomRouting::HasConnection(): satellite mullptr");
        switch (side.direction)
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
            return satellite->gateHalf("groundLink", cGate::Type::OUTPUT, side.gateIndex)->isConnectedOutside();
        default:
            error("HasConnection is not implemented for this side.");
        }
        return false;
    }
}