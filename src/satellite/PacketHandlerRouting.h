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
#include "metrics/MetricsCollector.h"
#include "mobility/INorad.h"
#include "mobility/NoradA.h"
#include "routing/RoutingFrame_m.h"
#include "routing/directed/DirectedRouting.h"
#include "routing/random/RandomRouting.h"

using namespace omnetpp;

namespace flora {

class PacketHandlerRouting : public cSimpleModule {
   protected:
    int satIndex = -1;
    int maxHops = -1;

    DirectedRouting *routing = nullptr;
    metrics::MetricsCollector *metricsCollector = nullptr;
    cMessage *selfMsg = nullptr;

   protected:
    virtual void initialize(int stage) override;
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    virtual void handleMessage(cMessage *msg) override;
    void processMessage(cMessage *msg);
    void receiveMessage(cMessage *msg);
    void routeMessage(cGate *gate, cMessage *msg);

    void insertSatinRoute(Packet *pkt);
    bool isExpired(Packet *pkt);
};

}  // namespace flora

#endif  // __FLORA_SATELLITE_PACKETHANDLERROUTING_H_