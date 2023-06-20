/*
 * PacketHandlerRouting.cc
 *
 *  Created on: Feb 08, 2022
 *      Author: Robin Ohs
 */

#include "ISLInterface.h"

#include "inet/common/packet/printer/PacketPrinter.h"

namespace flora {
namespace satellite {

Define_Module(ISLInterface);

void ISLInterface::initialize(int stage) {
    ClockUserModuleMixin::initialize(stage);
    if (stage == inet::INITSTAGE_LOCAL) {
        collectionTimer = new ClockEvent("CollectionTimer");
        islOutGate = gate("islOut");
    }
}

void ISLInterface::handleMessage(cMessage *msg) {
    // events
    if (msg == collectionTimer) {
        if (islOutGate->getTransmissionChannel()->isBusy()) {
            scheduleClockEventAfter(islOutGate->getTransmissionChannel()->getTransmissionFinishTime(), collectionTimer);
        } else if (provider->canPullSomePacket(inputGate->getPathStartGate())) {
            collectPacket();

            if (!collectionTimer->isScheduled() && provider->canPullSomePacket(inputGate->getPathStartGate())) {
                scheduleClockEventAfter(islOutGate->getTransmissionChannel()->getTransmissionFinishTime(), collectionTimer);
            }
        }
        return;
    }
}

void ISLInterface::collectPacket() {
    auto pkt = provider->pullPacket(inputGate->getPathStartGate());
    take(pkt);

    send(pkt, "islOut");
}

void ISLInterface::handleCanPullPacketChanged(cGate *gate) {
    Enter_Method("handleCanPullPacketChanged");
    // Simulates processing delay
    // if there is currently no packet in "processing", schedule the processing of the next package
    if (!collectionTimer->isScheduled()) {
        if (islOutGate->getTransmissionChannel()->isBusy()) {
            scheduleClockEventAfter(islOutGate->getTransmissionChannel()->getTransmissionFinishTime(), collectionTimer);
        } else if (provider->canPullSomePacket(inputGate->getPathStartGate())) {
            collectPacket();
        }
    }
}

void ISLInterface::handlePullPacketProcessed(Packet *packet, cGate *gate, bool successful) {
    Enter_Method("handlePullPacketProcessed");
}

const char *ISLInterface::resolveDirective(char directive) const {
    static std::string result;
    switch (directive) {
        case 'p':
            result = std::to_string(numProcessedPackets);
            break;
        case 'l':
            result = processedTotalLength.str();
            break;
        default:
            throw cRuntimeError("Unknown directive: %c", directive);
            break;
    }
    return result.c_str();
}

}  // namespace satellite
}  // namespace flora
