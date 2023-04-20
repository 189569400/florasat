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

#include "inet/common/packet/Packet.h"
#include "topologycontrol/TopologyControl.h"

namespace flora {
namespace routing {

using namespace omnetpp;
using namespace inet;
using namespace core::isldirection;

class RoutingBase : public cSimpleModule {
   protected:
    topologycontrol::TopologyControl *topologyControl = nullptr;

   public:
    virtual ISLDirection RoutePacket(Packet *pkt, cModule *callerSat) = 0;

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
