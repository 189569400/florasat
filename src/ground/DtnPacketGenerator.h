/*
 * PacketGenerator.h
 *
 *  Created on: May 27, 2023
 *      Author: Sebastian Montoya
 */

#ifndef __FLORA_GROUND_DTNPACKETGENERATOR_H_
#define __FLORA_GROUND_DTNPACKETGENERATOR_H_

#include <stdio.h>
#include <string.h>
#include <omnetpp.h>
#include "TransportHeader_m.h"
#include "core/Utils.h"
#include "inet/common/INETDefs.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/Simsignals.h"
#include "inet/common/packet/Message.h"
#include "inet/common/packet/Packet.h"
#include "metrics/MetricsCollector.h"
#include "networklayer/ConstellationRoutingTable.h"
#include "routing/RoutingBase.h"
#include "routing/RoutingHeader_m.h"
#include "topologycontrol/TopologyControlBase.h"
#include "routing/dtn/MsgTypes.h"
#include "routing/DtnRoutingHeader_m.h"
#include "DtnTransportHeader_m.h"
#include "routing/dtn/RoutingCgrModelRev17.h"
#include "routing/dtn/CustodyModel.h"

using namespace omnetpp;
using namespace inet;

namespace flora {

class DtnPacketGenerator : public cSimpleModule {
    public:
        virtual std::vector<int> getBundlesNumberVec();
        virtual std::vector<int> getDestinationEidVec();
        virtual std::vector<int> getSizeVec();
        virtual std::vector<double> getStartVec();
    protected:
        // statistics
        int numReceived = 0;
        int numSent = 0;
        B sentBytes = B(0);
        B receivedBytes = B(0);

        cHistogram hopCountStats;
        cOutVector hopCountVector;

        topologycontrol::TopologyControlBase *topologycontrol;
        routing::RoutingCgrModelRev17 *routingModule;
        networklayer::ConstellationRoutingTable *routingTable;
        ContactPlan *contactPlan;

    protected:
        virtual void initialize(int stage) override;
        virtual void finish() override;
        virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }

        int sentPackets;
        int groundStationId;
        int numGroundStations;
        int numSatellites;

    protected:
        virtual void handleMessage(cMessage *msg) override;
        void encapsulate(Packet *packet, int dstGs, int firstSat, int lastSat);
        void decapsulate(Packet *packet);

    private:
        std::vector<int> bundlesNumber;
        std::vector<int> destinationsEid;
        std::vector<int> sizes;
        std::vector<double> starts;
        // Forwarding threads
        map<int, ForwardingMsgStart *> forwardingMsgs_;
        routing::SdrModel sdr_;
        void parseBundlesNumber();
        void parseDestinationsEid();
        void parseSizes();
        void parseStarts();
        routing::CustodyModel custodyModel_;
        void dispatchBundle(DtnRoutingHeader *bundle);
        void refreshForwarding();
};

}  // namespace flora

#endif  // __FLORA_GROUND_DTNPACKETGENERATOR_H_
