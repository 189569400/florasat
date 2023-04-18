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
#include "routing/RoutingHeader_m.h"
#include "routing/RoutingBase.h"
#include "routing/random/RandomRouting.h"

using namespace omnetpp;

namespace flora {
namespace satellite {

class PacketHandlerRouting : public cSimpleModule {
   protected:
    int satIndex = -1;
    int maxHops = -1;

    routing::RoutingBase *routing = nullptr;
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

}  // namespace satellite
}  // namespace flora

#endif  // __FLORA_SATELLITE_PACKETHANDLERROUTING_H_