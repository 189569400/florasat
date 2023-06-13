/*
 * DiscoRouting.h
 *
 * Created on: Feb 04, 2023
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
enum SendDirection {
    LEFT_UP = 0,
    LEFT_DOWN = 1,
    RIGHT_UP = 2,
    RIGHT_DOWN = 3,
};

struct SendInformationRes {
    SendDirection dir;
    int hHorizontal;
    int hVertical;
};

enum DiscoMode {
    A2A = 0,
    D2D = 1,
    A2D = 2,
    D2A = 3,
};

class DiscoRouting : public RoutingBase {
   protected:
    int broadcastId = 0;
    SendInformationRes getSendInformation(const core::MinHopsRes& res);
    std::vector<int> discoRoute(DiscoMode mode, const topologycontrol::TopologyControlBase* tC, int src, int dst);
    std::vector<int> discoRouteA2A(const topologycontrol::TopologyControlBase* tC, int src, int dst);
    std::vector<int> discoRouteD2D(const topologycontrol::TopologyControlBase* tC, int src, int dst);
    std::vector<int> discoRouteA2D(const topologycontrol::TopologyControlBase* tC, int src, int dst);
    std::vector<int> discoRouteD2A(const topologycontrol::TopologyControlBase* tC, int src, int dst);

   public:
    void initialize(int stage) override;
    virtual void handleTopologyChange() override;
    ISLDirection routePacket(inet::Ptr<const RoutingHeader> rh, SatelliteRouting* callerSat) override;
};

}  // namespace routing
}  // namespace flora

#endif  // __FLORA_ROUTING_DISCOROUTING_H_
