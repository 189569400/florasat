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
    } else if (stage == inet::INITSTAGE_APPLICATION_LAYER) {
        routing = check_and_cast<routing::RoutingBase *>(getSystemModule()->getSubmodule("routing"));
        if (routing == nullptr) {
            error("Error in PacketHandlerRouting::initialize(): Routing is nullptr.");
        }
        INorad *noradModule = check_and_cast<INorad *>(getParentModule()->getSubmodule("NoradModule"));
        if (NoradA *noradAModule = dynamic_cast<NoradA *>(noradModule)) {
            satIndex = noradAModule->getSatelliteNumber();
        }
    }
}

void PacketHandlerRouting::handleMessage(cMessage *msg) {
    auto pkt = check_and_cast<inet::Packet *>(msg);
    if (msg->arrivedOn("queueIn")) {
        routeMessage(pkt);
    } else if (msg->isSelfMessage()) {
        auto frame = pkt->removeAtFront<ChannelBusyHeader>();
        for (cModule::GateIterator it(this); !it.end(); ++it) {
            cGate *gate = *it;
            if (frame->getGate() == gate->getId()) {
                sendMessage(gate, pkt);
                return;
            }
        }
        error("Error in PacketHandlerRouting::handleMessage: Unexpected gate %d not found.", frame->getGate());
    } else {
        if (msg->arrivedOn("groundLink1$i")) {
            routing->initRouting(pkt, getParentModule());
        }
        send(pkt, "queueOut");
    }
}

void PacketHandlerRouting::routeMessage(Packet *pkt) {
    auto frame = pkt->removeAtFront<RoutingHeader>();
    int hops = frame->getNumHop();
    int sequenceNumber = frame->getSequenceNumber();
    frame->appendRoute(satIndex);
    if (hops > maxHops) {
        EV << "SAT [" << satIndex << "]: MSG expired. Delete." << endl;
        delete pkt;
        return;
    }
    frame->setNumHop(frame->getNumHop() + 1);
    pkt->insertAtFront(frame);
    cGate *outputGate = nullptr;
    auto routeInformation = routing->routePacket(pkt, getParentModule());
    switch (routeInformation.direction) {
        case isldirection::Direction::ISL_DOWN:
            outputGate = gate("down1$o");
            break;
        case isldirection::Direction::ISL_UP:
            outputGate = gate("up1$o");
            break;
        case isldirection::Direction::ISL_LEFT:
            outputGate = gate("left1$o");
            break;
        case isldirection::Direction::ISL_RIGHT:
            outputGate = gate("right1$o");
            break;
        case isldirection::Direction::ISL_DOWNLINK:
            outputGate = gate("groundLink1$o", routeInformation.gateIndex);
            break;
        default:
            error("Unexpected gate");
    }
    sendMessage(outputGate, pkt);
}

void PacketHandlerRouting::sendMessage(cGate *gate, Packet *pkt) {
    if (gate->getTransmissionChannel()->isBusy()) {
        auto frame = makeShared<ChannelBusyHeader>();
        frame->setChunkLength(B(1));
        frame->setGate(gate->getId());
        pkt->insertAtFront(frame);
        EV << "Channel busy! ATTENTION!" << endl;
        scheduleAt(gate->getTransmissionChannel()->getTransmissionFinishTime(), pkt);
    } else {
        send(pkt->dup(), gate);
    }
}

}  // namespace satellite
}  // namespace flora
