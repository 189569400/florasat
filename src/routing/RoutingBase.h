/*
 * RoutingBase.h
 *
 * Created on: Feb 04, 2023
 *     Author: Robin Ohs
 */

#ifndef __FLORA_ROUTING_ROUTINGBASE_H_
#define __FLORA_ROUTING_ROUTINGBASE_H_

#include <omnetpp.h>

#include "ISLDirection.h"
#include "inet/common/packet/Packet.h"

using namespace omnetpp;

namespace flora {

class RoutingBase : public cSimpleModule {
   public:
    virtual ISLDirection RoutePacket(inet::Packet *pkt, cModule *callerSat);

   protected:
    virtual bool HasConnection(cModule *satellite, ISLDirection side);
};

}  // namespace flora

#endif  // __FLORA_ROUTING_ROUTINGBASE_H_