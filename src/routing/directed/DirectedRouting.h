/*
 * DirectedRouting.h
 *
 * Created on: Feb 04, 2023
 *     Author: Robin Ohs
 */

#ifndef ROUTING_DIRECTEDROUTING_H
#define ROUTING_DIRECTEDROUTING_H

#include <omnetpp.h>
#include "inet/common/packet/Packet.h"
#include "routing/RoutingFrame_m.h"
#include "routing/ISLDirection.h"
#include "topologycontrol/TopologyControl.h"

using namespace omnetpp;

namespace flora
{
    class DirectedRouting : public cSimpleModule
    {
        public:
            virtual void initialize(int stage) override;
            virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
            ISLDirection RoutePacket(inet::Packet *pkt, cModule *callerSat);
            bool HasConnection(cModule* satellite, ISLDirection side);
            bool IsSatelliteAscending(cModule* satellite);

        protected:
            topologycontrol::TopologyControl* topologyControl = nullptr;
    };
} // flora

#endif