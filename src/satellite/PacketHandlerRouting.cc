/*
 * PacketHandlerRouting.cc
 *
 *  Created on: Feb 08, 2022
 *      Author: Robin Ohs
 */

#include "PacketHandlerRouting.h"

namespace flora {
namespace satellite {

Define_Module(PacketHandlerRouting);

void PacketHandlerRouting::initialize(int stage) {
    if (stage == inet::INITSTAGE_LOCAL) {
        maxHops = par("maxHops");
        parentModule = check_and_cast<SatelliteRouting *>(getParentModule());
        tc = check_and_cast<TopologyControlBase *>(getSystemModule()->getSubmodule("topologyControl"));
        routing = check_and_cast<routing::RoutingBase *>(getSystemModule()->getSubmodule("routing"));
    } else if (stage == inet::INITSTAGE_APPLICATION_LAYER) {
        INorad *noradModule = check_and_cast<INorad *>(getParentModule()->getSubmodule("NoradModule"));
        if (NoradA *noradAModule = dynamic_cast<NoradA *>(noradModule)) {
            satIndex = noradAModule->getSatelliteNumber();
        }
    }
}

void PacketHandlerRouting::handleMessage(cMessage *msg) {
    auto pkt = check_and_cast<Packet *>(msg);
    // if (msg->arrivedOn("queueIn")) {
    //     routeMessage(pkt);
    // }
    // // If msg is self msg, it's because of a busy channel.
    // // Routing was done already, try to send it again to gate given by routing.
    // else if (msg->isSelfMessage()) {
    //     auto frame = pkt->removeAtFront<ChannelBusyHeader>();
    //     for (cModule::GateIterator it(this); !it.end(); ++it) {
    //         cGate *gate = *it;
    //         if (frame->getGate() == gate->getId()) {
    //             sendMessage(gate, pkt);
    //             return;
    //         }
    //     }
    //     error("Error in PacketHandlerRouting::handleMessage: Unexpected gate %d not found.", frame->getGate());
    // }
    // // if msg arrived on an ISL gate
    // else {
    //     ASSERT(msg->arrivedOn("groundLink1$i") || msg->arrivedOn("left1$i") || msg->arrivedOn("up1$i") || msg->arrivedOn("right1$i") || msg->arrivedOn("down1$i"));

    //     // if comes from ground, this is first sat and we need to initialite routing
    //     // For some algorithms this method does nothing.
    //     if (msg->arrivedOn("groundLink1$i")) {
    //         routing->initRouting(pkt, getParentModule());
    //     }
    //     // queue packet
    //     emit(packetReceivedSignal, pkt);
    //     send(pkt, "queueOut");
    // }
    if (msg->isSelfMessage()) {
        routeMessage(pkt);
    } else if (pkt->arrivedOn("queueIn")) {
        auto frame = pkt->removeAtFront<RoutingHeader>();

        // check if max hops reached
        if (frame->getNumHop() >= maxHops) {
            PacketDropDetails details;
            details.setReason(PacketDropReason::HOP_LIMIT_REACHED);
            emit(packetDroppedSignal, pkt, &details);
            delete pkt;
            return;
        }

        // set meta data
        frame->setNumHop(frame->getNumHop() + 1);
        frame->appendRoute(satIndex);
        pkt->insertAtFront(frame);

        routeMessage(pkt);
    } else {
        emit(packetReceivedSignal, pkt);
        send(pkt, "queueOut");
    }
}

void PacketHandlerRouting::routeMessage(Packet *pkt) {
    auto frame = pkt->peekAtFront<RoutingHeader>();
    auto dir = routing->routePacket(frame, parentModule);
    auto outputGate = getGate(dir, frame->getDestinationGroundstation());
    sendMessage(outputGate, pkt);
}

cGate *PacketHandlerRouting::getGate(isldirection::Direction dir, int gsId) {
    cGate *outputGate = nullptr;
    switch (dir) {
        case isldirection::Direction::ISL_DOWN: {
            outputGate = gate("down1$o");
            break;
        }
        case isldirection::Direction::ISL_UP: {
            outputGate = gate("up1$o");
            break;
        }
        case isldirection::Direction::ISL_LEFT: {
            outputGate = gate("left1$o");
            break;
        }
        case isldirection::Direction::ISL_RIGHT: {
            outputGate = gate("right1$o");
            break;
        }
        case isldirection::Direction::ISL_DOWNLINK: {
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

void PacketHandlerRouting::sendMessage(cGate *gate, Packet *pkt) {
    if (gate->getTransmissionChannel()->isBusy()) {
        scheduleAt(gate->getTransmissionChannel()->getTransmissionFinishTime(), pkt);
    } else {
        send(pkt, gate);
    }
}

}  // namespace satellite
}  // namespace flora
