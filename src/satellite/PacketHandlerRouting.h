/*
 * PacketHandlerRouting.h
 *
 *  Created on: Feb 08, 2023
 *      Author: Robin Ohs
 */

#ifndef SATELLITE_PACKETHANDLERROUTING_H_
#define SATELLITE_PACKETHANDLERROUTING_H_

#include <omnetpp.h>
#include "routing/random/RandomRouting.h"
#include "routing/directed/DirectedRouting.h"
#include "inet/common/INETDefs.h"
#include "routing/RoutingFrame_m.h"
#include "mobility/NoradA.h"
#include "mobility/INorad.h"

using namespace omnetpp;

namespace flora
{
    class PacketHandlerRouting : public cSimpleModule
    {
        protected:
            int satIndex = -1;
            int maxHops = -1;

            DirectedRouting* routing = nullptr;
            cMessage* selfMsg = nullptr;

        protected:
            virtual void initialize(int stage) override;
            virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
            virtual void handleMessage(cMessage *msg) override;
            void insertSatinRoute(Packet *pkt);
            bool isExpired(Packet *pkt);
    };

} // flora

#endif