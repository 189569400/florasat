/*
 * PacketGenerator.h
 *
 *  Created on: Feb 08, 2023
 *      Author: Robin Ohs
 */

#ifndef GROUND_PACKETGENERATOR_H_
#define GROUND_PACKETGENERATOR_H_

#include <omnetpp.h>
#include "routing/RoutingFrame_m.h"
#include "inet/common/INETDefs.h"
#include "inet/common/packet/Packet.h"
#include "inet/common/ModuleAccess.h"

using namespace omnetpp;

namespace flora
{

    class PacketGenerator : public cSimpleModule
    {
        protected:
            virtual void initialize(int stage) override;
            virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
            

            int groundStationId;
            int sentPackets = 0;
            int receivedPackets = 0;
            int numGroundStations;

            simtime_t updateInterval;

            cMessage *selfMsg = nullptr;

        protected:
            virtual void handleMessage(cMessage* msg) override;
            void handleSelfMessage(cMessage* msg);
            void receiveMessage(cMessage* pkt);
            void scheduleUpdate();
            int getRandomNumber();
    };

} // flora

#endif // GROUND_PACKETGENERATOR_H_
