/*
 * DDRARouting.h
 *
 * Created on: Jun 17, 2023
 *     Author: Robin Ohs
 */

#ifndef __FLORA_ROUTING_DDRAROUTING_H_
#define __FLORA_ROUTING_DDRAROUTING_H_

#include <omnetpp.h>

#include <vector>

#include "core/utils/SetUtils.h"
#include "core/utils/VectorUtils.h"
#include "inet/common/clock/ClockUserModuleMixin.h"
#include "inet/common/packet/Packet.h"
#include "routing/ForwardingTable.h"
#include "routing/RoutingBase.h"
#include "routing/RoutingHeader_m.h"
#include "routing/core/DijkstraShortestPath.h"

namespace flora {
namespace routing {

struct SatelliteState {
    bool congestedMsgSent = false;
    std::set<int> congestedSats = std::set<int>();
};

class DDRARouting : public ClockUserModuleMixin<RoutingBase> {
   protected:
    int broadcastId = 0;
    int queueThreshold = -1;
    std::unordered_map<int, SatelliteState> state;
    double timeSliceInterval = -1;
    ClockEvent *timeSliceTimer = nullptr;

   public:
    void initialize(int stage) override;
    void handleTopologyChange() override;
    void handleMessage(cMessage *msg) override;
    void handleMessage(inet::Packet *pkt, SatelliteRouting *callerSat) override;

    inet::Packet *handlePacketDrop(const inet::Packet *pkt, SatelliteRouting *callerSat, inet::PacketDropReason reason) override;
    inet::Packet *handleQueueSize(SatelliteRouting *callerSat, int queueSize, int maxQueueSize) override;

    ISLDirection routePacket(inet::Ptr<const RoutingHeader> rh, SatelliteRouting *callerSat) override;

   protected:
    void calculateRoutingTable(std::vector<std::vector<int>> &costMatrix, int satId);
    inet::Packet *createTransportPacket(int srcId);
};

}  // namespace routing
}  // namespace flora

#endif  // __FLORA_ROUTING_DDRAROUTING_H_
