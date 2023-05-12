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
#include "topologycontrol/TopologyControl.h"

namespace flora {
namespace routing {

using namespace omnetpp;
using namespace inet;
using namespace isldirection;

class RoutingBase : public cSimpleModule {
   protected:
    topologycontrol::TopologyControl *topologyControl = nullptr;

   public:
    virtual void initRouting(Packet *pkt, cModule *callerSat);
    virtual std::pair<int, int> calculateFirstAndLastSatellite(int srcGs, int dstGs);
    virtual ISLDirection routePacket(inet::Ptr<RoutingHeader> frame, cModule *callerSat) = 0;

   protected:
    virtual void initialize(int stage) override;
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    bool HasConnection(cModule *satellite, ISLDirection side);
    std::set<int> GetConnectedSatellites(int groundStationId);
    int GetGroundlinkIndex(int satelliteId, int groundstationId);
};

}  // namespace routing
}  // namespace flora

#endif  // __FLORA_ROUTING_ROUTINGBASE_H_
