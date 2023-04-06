/*
 * DirectedRouting.h
 *
 * Created on: Feb 04, 2023
 *     Author: Robin Ohs
 */

#ifndef __FLORA_ROUTING_DIRECTEDROUTING_H_
#define __FLORA_ROUTING_DIRECTEDROUTING_H_

#include <omnetpp.h>

#include "inet/common/packet/Packet.h"
#include "routing/ISLDirection.h"
#include "routing/RoutingBase.h"
#include "routing/RoutingFrame_m.h"
#include "topologycontrol/TopologyControl.h"

namespace flora {
namespace routing {

class DirectedRouting : public RoutingBase, public omnetpp::cSimpleModule {
   public:
    virtual void initialize(int stage) override;
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    ISLDirection RoutePacket(inet::Packet* pkt, cModule* callerSat);
    bool IsSatelliteAscending(cModule* satellite);

   protected:
    topologycontrol::TopologyControl* topologyControl = nullptr;
};

}  // namespace routing
}  // namespace flora

#endif  // __FLORA_ROUTING_DIRECTEDROUTING_H_