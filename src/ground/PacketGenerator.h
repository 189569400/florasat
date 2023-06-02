/*
 * PacketGenerator.h
 *
 *  Created on: Feb 08, 2023
 *      Author: Robin Ohs
 */

#ifndef __FLORA_GROUND_PACKETGENERATOR_H_
#define __FLORA_GROUND_PACKETGENERATOR_H_

#include <omnetpp.h>

#include "GroundLinkTag_m.h"
#include "core/Utils.h"
#include "inet/common/INETDefs.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/Simsignals.h"
#include "inet/common/packet/Message.h"
#include "inet/common/packet/Packet.h"
#include "inet/linklayer/common/InterfaceTag_m.h"
#include "inet/networklayer/contract/INetfilter.h"
#include "inet/networklayer/ipv4/Ipv4Header_m.h"
#include "inet/queueing/base/ActivePacketSinkBase.h"
#include "metrics/MetricsCollector.h"
#include "networklayer/ConstellationRoutingTable.h"
#include "routing/RoutingBase.h"
#include "routing/RoutingHeader_m.h"
#include "routing/core/DijkstraShortestPath.h"
#include "topologycontrol/TopologyControlBase.h"

using namespace omnetpp;
using namespace inet;

namespace flora {

class PacketGenerator : public cSimpleModule {
   protected:
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void finish() override;

    virtual void handleMessage(cMessage *msg) override;
    void encapsulate(Packet *packet, int dstGs, int firstSat, int lastSat);
    void decapsulate(Packet *packet);

   protected:
    // statistics
    int numReceived = 0;
    int numSent = 0;
    B sentBytes = B(0);
    B receivedBytes = B(0);

    int sentPackets;
    int groundStationId;
    int numGroundStations;

    cHistogram hopCountStats;
    cOutVector hopCountVector;

    topologycontrol::TopologyControlBase *topologycontrol;
    routing::RoutingBase *routingModule;
    networklayer::ConstellationRoutingTable *routingTable;
};

}  // namespace flora

#endif  // __FLORA_GROUND_PACKETGENERATOR_H_
