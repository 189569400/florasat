/*
 * PacketHandlerRouting.h
 *
 *  Created on: Feb 08, 2023
 *      Author: Robin Ohs
 */

#ifndef __FLORA_SATELLITE_PACKETHANDLERROUTING_H_
#define __FLORA_SATELLITE_PACKETHANDLERROUTING_H_

#include <omnetpp.h>

#include "inet/common/INETDefs.h"
#include "inet/common/Simsignals.h"
#include "inet/common/clock/ClockUserModuleMixin.h"
#include "inet/queueing/base/ActivePacketSinkBase.h"
#include "inet/queueing/queue/PacketQueue.h"
#include "mobility/INorad.h"
#include "mobility/NoradA.h"
#include "routing/RoutingBase.h"
#include "routing/RoutingHeader_m.h"
#include "satellite/SatelliteRouting.h"
#include "topologycontrol/TopologyControlBase.h"

using namespace omnetpp;
using namespace inet::queueing;
using namespace flora::topologycontrol;

namespace flora {
namespace satellite {

class PacketHandlerRouting : public ClockUserModuleMixin<ActivePacketSinkBase> {
   protected:
    SatelliteRouting *parentModule = nullptr;        // cached pointer
    inet::queueing::PacketQueue *queue = nullptr;        // cached pointer
    flora::routing::RoutingBase *routing = nullptr;  // cached pointer
    TopologyControlBase *tc = nullptr;               // cached pointer

    int maxHops = -1;
    cPar *processingDelayParameter = nullptr;
    ClockEvent *collectionTimer = nullptr;
    int numDroppedPackets = -1;
    int satIndex = -1;
    int maxQueueSize = -1;

   protected:
    // cModule
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;

    // packet sink
    virtual void scheduleCollectionTimer();
    virtual void collectPacket();

    // packet handler
    void sendMessage(isldirection::ISLDirection dir, Packet *pkt, bool silent, int dstGs = -1);
    void dropPacket(Packet *msg, PacketDropReason reason, bool silent, int limit = -1);
    cGate *getGate(isldirection::ISLDirection routingResult, int gsId);

   public:
    virtual ~PacketHandlerRouting() { cancelAndDeleteClockEvent(collectionTimer); }
    void broadcastMessage(Packet *pkt);

    virtual void handleCanPullPacketChanged(cGate *gate) override;
    virtual void handlePullPacketProcessed(Packet *packet, cGate *gate, bool successful) override;

    const char *resolveDirective(char directive) const override;
};

}  // namespace satellite
}  // namespace flora

#endif  // __FLORA_SATELLITE_PACKETHANDLERROUTING_H_
