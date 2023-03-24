/*
 * SatelliteRouting.h
 *
 *  Created on: Mar 20, 2023
 *      Author: Robin Ohs
 */

#ifndef __FLORA_SATELLITE_SATELLITEROUTING_H_
#define __FLORA_SATELLITE_SATELLITEROUTING_H_

#include <omnetpp.h>
#include "inet/common/Simsignals.h"
#include "inet/common/packet/Packet.h"
#include "routing/RoutingFrame_m.h"
#include "inet/common/INETDefs.h"

using namespace omnetpp;
using namespace inet;

namespace flora
{
    namespace satellite
    {
        class SatelliteRouting : public cSimpleModule, cListener
        {
        public:
            virtual void receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details) override;
        
        protected:
            virtual void finish() override;
            virtual void initialize(int stage) override;

        private:
            void handlePacketDropped(inet::Packet* packet, inet::PacketDropDetails* reason);
        };
    } // namespace satellite

} // namespace flora

#endif // __FLORA_SATELLITE_SATELLITEROUTING_H_