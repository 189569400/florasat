/*
 * PacketHandlerRouting.cc
 *
 *  Created on: Feb 08, 2022
 *      Author: Robin Ohs
 */

#include "PacketHandlerRouting.h"

#include "inet/common/packet/printer/PacketPrinter.h"

namespace flora {
namespace satellite {

Define_Module(PacketHandlerRouting);

void PacketHandlerRouting::initialize(int stage) {
    ClockUserModuleMixin::initialize(stage);
    if (stage == inet::INITSTAGE_LOCAL) {
        maxHops = par("maxHops");
        processingDelayParameter = &par("processingDelay");
        collectionTimer = new ClockEvent("CollectionTimer");

        parentModule = check_and_cast<SatelliteRouting *>(getParentModule());
        tc = check_and_cast<TopologyControlBase *>(getSystemModule()->getSubmodule("topologyControl"));
        routing = check_and_cast<routing::RoutingBase *>(getSystemModule()->getSubmodule("routing"));

        satIndex = parentModule->getId();
        numDroppedPackets = 0;
        WATCH(numDroppedPackets);
    }
}

void PacketHandlerRouting::handleMessage(cMessage *msg) {
    // events
    if (msg == collectionTimer) {
        if (provider->canPullSomePacket(inputGate->getPathStartGate())) {
            collectPacket();
            // if there is another packet, start processing
            scheduleCollectionTimer();
        }
        return;
    }

    // packets
    auto pkt = check_and_cast<Packet *>(msg);

    if (msg->isSelfMessage()) {  // rescheduled message
        auto tag = pkt->removeTag<routing::NextGateTag>();
        auto dir = isldirection::ISLDirection(tag->getDir());
        auto reportDrop = tag->getReportDrop();
        sendMessage(dir, pkt, pkt->peekAtFront<RoutingHeader>()->getDstGs(), reportDrop);
    } else {
        emit(packetReceivedSignal, pkt);
        // queue packet
        send(pkt, "out");
    }
}

void PacketHandlerRouting::scheduleCollectionTimer() {
    ASSERT(!collectionTimer->isScheduled());
    scheduleClockEventAfter(processingDelayParameter->doubleValue(), collectionTimer);
}

void PacketHandlerRouting::collectPacket() {
    auto pkt = provider->pullPacket(inputGate->getPathStartGate());
    take(pkt);

    auto frame = pkt->removeAtFront<RoutingHeader>();

    // set meta data
    frame->setTTL(frame->getTTL() - 1);
    pkt->insertAtFront(frame);

    handlePacketProcessed(pkt);
    updateDisplayString();

    switch (frame->getType()) {
        case Type::G2G: {
            auto frame = pkt->peekAtFront<RoutingHeader>();
            auto dir = routing->routePacket(frame, parentModule);
            // if packet has not reached dst and ttl is 0 -> drop packet
            if (dir != isldirection::ISLDirection::GROUNDLINK && frame->getTTL() == 0) {
                dropPacket(pkt, PacketDropReason::HOP_LIMIT_REACHED);
            } else {
                sendMessage(dir, pkt, true, frame->getDstGs());
            }
            break;
        }
        case Type::S2S: {
            error("Unimplemented");
            break;
        }
        case Type::SBroadcast: {
            routing->handleMessage(pkt, parentModule);
            if (frame->getTTL() != 0) {
                broadcastMessage(pkt);
            }
            break;
        }
        default: {
            error("Unsupported type");
            break;
        }
    }
}

cGate *PacketHandlerRouting::getGate(isldirection::ISLDirection dir, int gsId) {
    cGate *outputGate = nullptr;
    switch (dir) {
        case isldirection::ISLDirection::DOWN: {
            outputGate = gate("down1$o");
            break;
        }
        case isldirection::ISLDirection::UP: {
            outputGate = gate("up1$o");
            break;
        }
        case isldirection::ISLDirection::LEFT: {
            outputGate = gate("left1$o");
            break;
        }
        case isldirection::ISLDirection::RIGHT: {
            outputGate = gate("right1$o");
            break;
        }
        case isldirection::ISLDirection::GROUNDLINK: {
            int gateIndex = routing->getGroundlinkIndex(satIndex, gsId);
            if (gateIndex == -1) error("No valid groundlink index found between %d and %d.", satIndex, gsId);
            outputGate = gate("groundLink1$o", gateIndex);
            break;
        }
        default: {
            error("Unexpected gate");
            break;
        }
    }
    ASSERT(outputGate != nullptr);
    return outputGate;
}

void PacketHandlerRouting::sendMessage(isldirection::ISLDirection dir, Packet *pkt, bool reportDrop, int dstGs) {
    auto gate = getGate(dir, dstGs);

    if (!gate->getNextGate()->isConnectedOutside()) {
        dropPacket(pkt, PacketDropReason::INTERFACE_DOWN, reportDrop);
        return;
    }

    if (gate->getTransmissionChannel()->isBusy()) {
        auto tag = pkt->addTagIfAbsent<routing::NextGateTag>();
        tag->setDir(routing::Dir(dir));
        tag->setReportDrop(reportDrop);
        scheduleAt(gate->getTransmissionChannel()->getTransmissionFinishTime(), pkt);
    } else {
        send(pkt, gate);
    }
}

void PacketHandlerRouting::broadcastMessage(Packet *pkt) {
    VALIDATE(pkt != nullptr);

    take(pkt);

    sendMessage(isldirection::ISLDirection::LEFT, pkt->dup(), false);
    sendMessage(isldirection::ISLDirection::UP, pkt->dup(), false);
    sendMessage(isldirection::ISLDirection::RIGHT, pkt->dup(), false);
    sendMessage(isldirection::ISLDirection::DOWN, pkt->dup(), false);
    delete pkt;
}

void PacketHandlerRouting::dropPacket(Packet *pkt, PacketDropReason reason, bool reportDrop, int limit) {
    if (reportDrop) {
        auto res = routing->handlePacketDrop(pkt, parentModule, reason);
        if (res != nullptr) {
            broadcastMessage(res);
        }
    }
    PacketProcessorBase::dropPacket(pkt, reason);
    numDroppedPackets++;
    updateDisplayString();
}

void PacketHandlerRouting::handleCanPullPacketChanged(cGate *gate) {
    Enter_Method("handleCanPullPacketChanged");
    // Simulates processing delay
    // if there is currently no packet in "processing", schedule the processing of the next package
    if (!collectionTimer->isScheduled()) {
        scheduleCollectionTimer();
    }
}

void PacketHandlerRouting::handlePullPacketProcessed(Packet *packet, cGate *gate, bool successful) {
    Enter_Method("handlePullPacketProcessed");
}

const char *PacketHandlerRouting::resolveDirective(char directive) const {
    static std::string result;
    switch (directive) {
        case 'p':
            result = std::to_string(numProcessedPackets);
            break;
        case 'd':
            result = std::to_string(numDroppedPackets);
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
