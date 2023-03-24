/*
 * RandomRouting.h
 *
 * Created on: Feb 04, 2023
 *     Author: Robin Ohs
 */

#ifndef __FLORA_ROUTING_RANDOMROUTING_H_
#define __FLORA_ROUTING_RANDOMROUTING_H_

#include <omnetpp.h>

#include "inet/common/packet/Packet.h"
#include "routing/ISLDirection.h"

using namespace omnetpp;

namespace flora {

class RandomRouting : public cSimpleModule {
   public:
    ISLDirection RoutePacket(inet::Packet *pkt, cModule *callerSat);
    bool HasConnection(cModule *satellite, ISLDirection side);
};

}  // namespace flora

#endif  // __FLORA_ROUTING_RANDOMROUTING_H_