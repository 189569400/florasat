/*
 * DsprRouting.h
 *
 * Created on: Feb 04, 2023
 *     Author: Robin Ohs
 */

#ifndef __FLORA_ROUTING_DSPRROUTING_H_
#define __FLORA_ROUTING_DSPRROUTING_H_

#include <omnetpp.h>

#include <vector>

#include "core/utils/VectorUtils.h"
#include "inet/common/packet/Packet.h"
#include "routing/ForwardingTable.h"
#include "routing/RoutingBase.h"
#include "routing/RoutingHeader_m.h"
#include "routing/core/DijkstraShortestPath.h"

namespace flora {
namespace routing {

class DsprRouting : public RoutingBase {
   public:
    virtual void handleTopologyChange() override;
    Direction routePacket(inet::Ptr<const RoutingHeader> rh, SatelliteRouting *callerSat) override;
};

}  // namespace routing
}  // namespace flora

#endif  // __FLORA_ROUTING_DSPRROUTING_H_
