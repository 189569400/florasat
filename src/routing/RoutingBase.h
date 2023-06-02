/*
 * RoutingBase.h
 *
 * Created on: Feb 04, 2023
 *     Author: Robin Ohs
 */

#ifndef __FLORA_ROUTING_ROUTINGBASE_H_
#define __FLORA_ROUTING_ROUTINGBASE_H_

#include <omnetpp.h>

#include <vector>

#include "RoutingHeader_m.h"
#include "core/utils/VectorUtils.h"
#include "inet/common/packet/Packet.h"
#include "routing/core/DijkstraShortestPath.h"
#include "satellite/SatelliteRouting.h"
#include "topologycontrol/TopologyControlBase.h"

namespace flora {
namespace routing {

using namespace omnetpp;
using namespace inet;
using namespace isldirection;

class RoutingBase : public cSimpleModule {
   protected:
    topologycontrol::TopologyControlBase *topologyControl = nullptr;

   public:
    virtual void handleTopologyChange(){};
    virtual void handlePacket(inet::Packet *pkt, SatelliteRouting *callerSat){};
    virtual Direction routePacket(inet::Ptr<const RoutingHeader> rh, SatelliteRouting *callerSat) = 0;
    virtual std::pair<int, int> calculateFirstAndLastSatellite(int srcGs, int dstGs);
    int getGroundlinkIndex(int satelliteId, int groundstationId);

   protected:
    virtual void initialize(int stage) override;
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    std::set<int> const &getConnectedSatellites(int groundStationId) const;
};

}  // namespace routing
}  // namespace flora

#endif  // __FLORA_ROUTING_ROUTINGBASE_H_
