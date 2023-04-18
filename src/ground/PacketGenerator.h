/*
 * PacketGenerator.h
 *
 *  Created on: Feb 08, 2023
 *      Author: Robin Ohs
 */

#ifndef __FLORA_GROUND_PACKETGENERATOR_H_
#define __FLORA_GROUND_PACKETGENERATOR_H_

#include <omnetpp.h>

#include "core/Utils.h"
#include "inet/common/INETDefs.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/Simsignals.h"
#include "inet/common/packet/Message.h"
#include "inet/common/packet/Packet.h"
#include "metrics/MetricsCollector.h"
#include "routing/RoutingHeader_m.h"

using namespace omnetpp;
using namespace inet;

namespace flora {

class PacketGenerator : public cSimpleModule {
   protected:
    // statistics
    int numReceived = 0;
    int numSent = 0;
    B sentBytes = B(0);
    B receivedBytes = B(0);

   protected:
    virtual void initialize(int stage) override;
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }

    int sentPackets;
    int groundStationId;
    int numGroundStations;

   protected:
    virtual void handleMessage(cMessage *msg) override;
    void encapsulate(Packet *packet, int destinationId);
    void decapsulate(Packet *packet);
};

}  // namespace flora

#endif  // __FLORA_GROUND_PACKETGENERATOR_H_
