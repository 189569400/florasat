/*
 * ActivePacketConsumer.hpp
 *
 *  Created on: Mar 17, 2023
 *      Author: Robin Ohs
 */

#ifndef SATELLITE_ACTIVEPACKETCONSUMER_H_
#define SATELLITE_ACTIVEPACKETCONSUMER_H_

#include "inet/common/clock/ClockUserModuleMixin.h"
#include "inet/queueing/sink/ActivePacketSink.h"

using namespace inet;
using namespace inet::queueing;

namespace flora
{
    class INET_API ActivePacketConsumer : public ActivePacketSink
    {
    protected:
        virtual void collectPacket();

    public:
        virtual ~ActivePacketConsumer() { cancelAndDeleteClockEvent(collectionTimer); }

        virtual void handleCanPullPacketChanged(cGate *gate) override;
    };
} // namespace flora
#endif SATELLITE_ACTIVEPACKETCONSUMER_H_