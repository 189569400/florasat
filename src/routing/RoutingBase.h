/*
 * RoutingBase.h
 *
 * Created on: Feb 04, 2023
 *     Author: Robin Ohs
 */

#ifndef ROUTING_ROUTINGBASE_H
#define ROUTING_ROUTINGBASE_H

#include <omnetpp.h>
#include "ISLDirection.h"

using namespace omnetpp;

namespace flora
{
    class RoutingBase : public cSimpleModule
    {
        public:
            virtual ISLDirection RoutePacket(cMessage *msg, cModule *callerSat);

        protected:
            bool HasConnection(cModule* satellite, ISLDirection side);
    };
} // flora

#endif