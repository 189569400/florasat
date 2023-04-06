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
#include "inet/common/packet/Packet.h"
#include "metrics/MetricsCollector.h"
#include "routing/RoutingFrame_m.h"

namespace flora {

class PacketGenerator : public omnetpp::cSimpleModule {
   protected:
    virtual void initialize(int stage) override;
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }

    int sentPackets;
    int groundStationId;
    int numGroundStations;

   protected:
    virtual void handleMessage(cMessage* msg) override;
    void sendMessage(inet::Packet* msg);
};

}  // namespace flora

#endif  // __FLORA_GROUND_PACKETGENERATOR_H_
