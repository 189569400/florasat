/*
 * RandomRouting.h
 *
 * Created on: Feb 04, 2023
 *     Author: Robin Ohs
 */

#ifndef ROUTING_RANDOMROUTING_H
#define ROUTING_RANDOMROUTING_H

#include <omnetpp.h>
#include "routing/RoutingBase.h"

using namespace omnetpp;

namespace flora
{
    class RandomRouting : public RoutingBase
    {
        public:
            virtual ISLDirection RoutePacket(cMessage *msg, cModule *callerSat) override;
    };
} // flora

#endif