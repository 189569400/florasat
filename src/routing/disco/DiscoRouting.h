/*
 * DiscoRouting.h
 *
 * Created on: Jun 11, 2023
 *     Author: Robin Ohs
 */

#ifndef __FLORA_ROUTING_DISCOROUTING_H_
#define __FLORA_ROUTING_DISCOROUTING_H_

#include <omnetpp.h>

#include <vector>

#include "core/utils/VectorUtils.h"
#include "inet/common/packet/Packet.h"
#include "routing/ForwardingTable.h"
#include "routing/RoutingBase.h"
#include "routing/RoutingHeader_m.h"
#include "routing/core/MinHopCount.h"

namespace flora {
namespace routing {

class DiscoRouting : public RoutingBase {
   protected:
    int broadcastId = 0;
    std::vector<int> discoRouteA2A(const core::MinHopsRes& minHops, const topologycontrol::TopologyControlBase* tC, int src, int dst);
    std::vector<int> discoRouteA2D(const core::MinHopsRes& minHops, const topologycontrol::TopologyControlBase* tC, int src, int dst);

   public:
    void initialize(int stage) override;
    virtual void handleTopologyChange() override;
    ISLDirection routePacket(inet::Ptr<const RoutingHeader> rh, SatelliteRouting* callerSat) override;
};

}  // namespace routing
}  // namespace flora

#endif  // __FLORA_ROUTING_DISCOROUTING_H_
