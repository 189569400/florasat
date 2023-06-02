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
        satIndex = parentModule->getId();
        tc = check_and_cast<TopologyControlBase *>(getSystemModule()->getSubmodule("topologyControl"));
        routing = check_and_cast<routing::RoutingBase *>(getSystemModule()->getSubmodule("routing"));
    }
}

void PacketHandlerRouting::handleMessage(cMessage *msg) {
    auto pkt = check_and_cast<Packet *>(msg);
    if (msg->isSelfMessage()) {
        routeMessage(pkt);
    } else if (pkt->arrivedOn("queueIn")) {
        auto frame = pkt->removeAtFront<RoutingHeader>();

        // check if max hops reached
        if (frame->getNumHop() >= maxHops) {
            dropPacket(pkt, PacketDropReason::HOP_LIMIT_REACHED);
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
    if (!gate->getNextGate()->isConnectedOutside()) {
        dropPacket(pkt, PacketDropReason::INTERFACE_DOWN);
        return;
    }
    if (gate->getTransmissionChannel()->isBusy()) {
        scheduleAt(gate->getTransmissionChannel()->getTransmissionFinishTime(), pkt);
    } else {
        send(pkt, gate);
    }
}

void PacketHandlerRouting::dropPacket(Packet *pkt, PacketDropReason reason) {
    PacketDropDetails details;
    details.setReason(reason);
    emit(packetDroppedSignal, pkt, &details);
    delete pkt;
}

}  // namespace satellite
}  // namespace flora
