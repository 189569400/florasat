/*
 * SatelliteRouting.cc
 *
 *  Created on: Mar 20, 2023
 *      Author: Robin Ohs
 */

#include "satellite/SatelliteRouting.hpp"

namespace flora
{
    namespace satellite
    {
        Define_Module(SatelliteRouting);

        void SatelliteRouting::initialize(int stage)
        {
            // subscribe to dropped packets
            subscribe(packetDroppedSignal, this);
        }

        void SatelliteRouting::finish()
        {
        }

        void SatelliteRouting::receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details)
        {
            if (signalID == packetDroppedSignal)
            {
                auto packet = check_and_cast<inet::Packet*>(obj);
                auto reason = check_and_cast<inet::PacketDropDetails*>(details);
                handlePacketDropped(packet, reason);
            }
        }

        void SatelliteRouting::handlePacketDropped(inet::Packet* packet, inet::PacketDropDetails* reason)
        {
            EV_INFO << "Dropped: " << packet << EV_ENDL;
        }

    } // namespace satellite

} // namespace flora