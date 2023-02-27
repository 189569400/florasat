/*
 * DirectedRouting.cc
 *
 * Created on: Feb 04, 2023
 *     Author: Robin Ohs
 */

#include "DirectedRouting.h"

namespace flora
{
    Define_Module(DirectedRouting);

    void DirectedRouting::initialize(int stage)
    {
        if (stage == inet::INITSTAGE_APPLICATION_LAYER)
        {
            topologyControl = check_and_cast<topologycontrol::TopologyControl *>(getSystemModule()->getSubmodule("topologyControl"));
            if (topologyControl == nullptr)
            {
                error("Error in DirectedRouting::initialize(): topologyControl is nullptr.");
            }
        }
    }

    ISLDirection DirectedRouting::RoutePacket(inet::Packet *pkt, cModule *callerSat)
    {
        auto frame = pkt->removeAtFront<RoutingFrame>();
        int destGroundstationId = frame->getDestinationGroundstation();
        pkt->insertAtFront(frame);

        int routingSatIndex = callerSat->getIndex();

        int destSatIndex = -1;
        for (int number : topologyControl->getGroundstationInfo(destGroundstationId)->satellites)
        {
            destSatIndex = number;
            break;
        }
        if(destSatIndex == - 1) {
            error("DestSatIndex was not set!");
        }

        if (routingSatIndex == destSatIndex)
        {
            // EV << "END SAT REACHED. SEND TO GROUND" << endl;
            return ISLDirection(Direction::ISL_DOWNLINK, 0);
        }

        int myPlane = topologyControl->calculateSatellitePlane(routingSatIndex);
        int destPlane = topologyControl->calculateSatellitePlane(destSatIndex);
        bool isAscending = IsSatelliteAscending(callerSat);

        // destination is on same plane
        if (myPlane < destPlane)
        {
            if (isAscending)
            {
                if (HasConnection(callerSat, ISLDirection(Direction::ISL_RIGHT, -1)))
                {
                    return ISLDirection(Direction::ISL_RIGHT, -1);
                }
                else
                {
                    return ISLDirection(Direction::ISL_DOWN, -1);
                }
            }
            else
            {
                if (HasConnection(callerSat, ISLDirection(Direction::ISL_LEFT, -1)))
                {
                    return ISLDirection(Direction::ISL_LEFT, -1);
                }
                else
                {
                    return ISLDirection(Direction::ISL_UP, -1);
                }
            }
        }
        else if (myPlane > destPlane)
        {
            if (isAscending)
            {
                if (HasConnection(callerSat, ISLDirection(Direction::ISL_LEFT, -1)))
                {
                    return ISLDirection(Direction::ISL_LEFT, -1);
                }
                else
                {
                    return ISLDirection(Direction::ISL_DOWN, -1);
                }
            }
            else
            {
                if (HasConnection(callerSat, ISLDirection(Direction::ISL_RIGHT, -1)))
                {
                    return ISLDirection(Direction::ISL_RIGHT, -1);
                }
                else
                {
                    return ISLDirection(Direction::ISL_UP, -1);
                }
            }
        }
        else if (myPlane == destPlane)
        {
            if (routingSatIndex < destSatIndex && HasConnection(callerSat, ISLDirection(Direction::ISL_UP, -1)))
            {
                return ISLDirection(Direction::ISL_UP, -1);
            }
            else if (routingSatIndex > destSatIndex && HasConnection(callerSat, ISLDirection(Direction::ISL_DOWN, -1)))
            {
                return ISLDirection(Direction::ISL_DOWN, -1);
            }
            else
            {
                error("Error in PacketHandlerDirected::handleMessage: choosen gate should be connected!");
            }
        }
    }

    bool DirectedRouting::IsSatelliteAscending(cModule *satellite)
    {
        NoradA *noradA = check_and_cast<NoradA *>(satellite->getSubmodule("NoradModule"));
        if (noradA == nullptr)
        {
            error("Error in TopologyControl::getSatellites(): noradA module of loRaGW with index %zu is nullptr. Make sure a module with name `NoradModule` exists.", satellite->getIndex());
        }
        return noradA->isAscending();
    }

    bool DirectedRouting::HasConnection(cModule *satellite, ISLDirection side)
    {
        if (satellite == nullptr)
            error("DirectedRouting::HasConnection(): satellite mullptr");
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