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

class ISLInterface : public ClockUserModuleMixin<ActivePacketSinkBase> {
   protected:
    ClockEvent *collectionTimer = nullptr;
    cGate *islOutGate = nullptr;

   protected:
    // cModule
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;

    // packet sink
    virtual void collectPacket();

   public:
    virtual ~ISLInterface() { cancelAndDeleteClockEvent(collectionTimer); }

    virtual void handleCanPullPacketChanged(cGate *gate) override;
    virtual void handlePullPacketProcessed(Packet *packet, cGate *gate, bool successful) override;

    const char *resolveDirective(char directive) const override;
};

}  // namespace satellite
}  // namespace flora

#endif  // __FLORA_SATELLITE_PACKETHANDLERROUTING_H_
