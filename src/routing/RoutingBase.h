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

namespace flora {
namespace routing {

class RoutingBase {
   public:
    virtual ISLDirection RoutePacket(inet::Packet *pkt, omnetpp::cModule *callerSat) = 0;

   protected:
    virtual bool HasConnection(omnetpp::cModule *satellite, ISLDirection side);
};

}  // namespace routing
}  // namespace flora

#endif  // __FLORA_ROUTING_ROUTINGBASE_H_