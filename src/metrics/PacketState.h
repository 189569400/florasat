/*
 * PacketState.h
 *
 *  Created on: Mar 13, 2023
 *      Author: Robin Ohs
 */

#ifndef METRICS_PACKETSTATE_H_
#define METRICS_PACKETSTATE_H_

#include <string>
#include <omnetpp.h>

namespace flora
{
    namespace metrics
    {
        namespace PacketState
        {
            /** @brief Enum to indicate the state of a packet that will be recorded. */
            enum Type
            {
                DELIVERED,
                WRONG_DELIVERED,
                DROPPED,
                UNROUTABLE,
            };

            static const Type All[] = { DELIVERED, WRONG_DELIVERED, DROPPED, UNROUTABLE };

            std::string to_string(PacketState::Type state);

        } // namespace PacketState

    } // namespace metrics

} // namespace flora

#endif // METRICS_PACKETSTATE_H_