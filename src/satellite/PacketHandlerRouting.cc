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
        parentModule = check_and_cast<SatelliteRouting *>(getParentModule());
        queue = check_and_cast<inet::queueing::PacketQueue *>(getParentModule()->getSubmodule("queue"));
        routing = check_and_cast<routing::RoutingBase *>(getSystemModule()->getSubmodule("routing"));
        tc = check_and_cast<TopologyControlBase *>(getSystemModule()->getSubmodule("topologyControl"));

        maxHops = par("maxHops");
        processingDelayParameter = &par("processingDelay");
        collectionTimer = new ClockEvent("CollectionTimer");
        numDroppedPackets = 0;
        WATCH(numDroppedPackets);
        satIndex = parentModule->getId();
        maxQueueSize = parentModule->getSubmodule("queue")->par("packetCapacity");
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
    } else {
        // queue packets
        inet::Packet *res = routing->handleQueueSize(parentModule, queue->getNumPackets(), maxQueueSize);
        if (res != nullptr) {
            broadcastMessage(res);
        }
        send(msg, "out");
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
                dropPacket(pkt, PacketDropReason::HOP_LIMIT_REACHED, false);
            } else {
                sendMessage(dir, pkt, false, frame->getDstGs());
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
            outputGate = gate("downOut");
        } break;
        case isldirection::ISLDirection::UP: {
            outputGate = gate("upOut");
        } break;
        case isldirection::ISLDirection::LEFT: {
            outputGate = gate("leftOut");
        } break;
        case isldirection::ISLDirection::RIGHT: {
            outputGate = gate("rightOut");
        } break;
        case isldirection::ISLDirection::GROUNDLINK: {
            int gateIndex = routing->getGroundlinkIndex(satIndex, gsId);
            if (gateIndex == -1) error("No valid groundlink index found between %d and %d.", satIndex, gsId);
            outputGate = gate("groundLinkOut", gateIndex);
        } break;
        default: {
            error("Unexpected gate");
        } break;
    }
    ASSERT(outputGate != nullptr);
    return outputGate;
}

void PacketHandlerRouting::sendMessage(isldirection::ISLDirection dir, Packet *pkt, bool silent, int dstGs) {
    bool drop = false;
    switch (dir) {
        case isldirection::ISLDirection::LEFT: {
            if (!parentModule->gate("leftOut")->isConnectedOutside()) {
                EV << "DROP PACKET -> left not connected" << endl;
                drop = true;
            }
        } break;
        case isldirection::ISLDirection::UP: {
            if (!parentModule->gate("upOut")->isConnectedOutside()) {
                EV << "DROP PACKET -> up not connected" << endl;
                drop = true;
            }
        } break;
        case isldirection::ISLDirection::RIGHT: {
            if (!parentModule->gate("rightOut")->isConnectedOutside()) {
                EV << "DROP PACKET -> right not connected" << endl;
                drop = true;
            }
        } break;
        case isldirection::ISLDirection::DOWN: {
            if (!parentModule->gate("downOut")->isConnectedOutside()) {
                EV << "DROP PACKET -> dopwn not connected" << endl;
                drop = true;
            }
        } break;
        case isldirection::ISLDirection::GROUNDLINK: {
            // intentionally blank, as groundlink has no state currently
        } break;
    }
    if (drop) {
        dropPacket(pkt, PacketDropReason::INTERFACE_DOWN, silent);
    } else {
        cGate *gatePtr = getGate(dir, dstGs);
        send(pkt, gatePtr);
    }
}

void PacketHandlerRouting::broadcastMessage(Packet *pkt) {
    VALIDATE(pkt != nullptr);

    take(pkt);
    if (parentModule->hasLeftSat()) {
        sendMessage(isldirection::ISLDirection::LEFT, pkt->dup(), true);
    }
    if (parentModule->hasUpSat()) {
        sendMessage(isldirection::ISLDirection::UP, pkt->dup(), true);
    }
    if (parentModule->hasRightSat()) {
        sendMessage(isldirection::ISLDirection::RIGHT, pkt->dup(), true);
    }
    if (parentModule->hasDownSat()) {
        sendMessage(isldirection::ISLDirection::DOWN, pkt->dup(), true);
    }
    delete pkt;
}

void PacketHandlerRouting::dropPacket(Packet *pkt, PacketDropReason reason, bool silent, int limit) {
    if (!silent) {
        inet::Packet *res = routing->handlePacketDrop(pkt, parentModule, reason);
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
