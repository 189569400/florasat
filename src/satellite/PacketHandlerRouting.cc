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
    if (msg->arrivedOn("queueIn")) {
        processMessage(msg);
    } else {
        receiveMessage(msg);
    }
}

void PacketHandlerRouting::processMessage(cMessage *msg) {
    auto pkt = check_and_cast<inet::Packet *>(msg);

    auto frame = pkt->removeAtFront<RoutingHeader>();
    int hops = frame->getNumHop();
    int sequenceNumber = frame->getSequenceNumber();
    pkt->insertAtFront(frame);

    if (hops > maxHops) {
        EV << "SAT [" << satIndex << "]: MSG expired. Delete." << endl;
        // TODO: EMIT PACKET DROP
        bubble("MSG expired.");
        delete msg;
        return;
    }

    insertSatinRoute(pkt);

    cGate *outputGate = nullptr;
    auto routeInformation = routing->RoutePacket(pkt, getParentModule());
    switch (routeInformation.direction) {
        case Direction::ISL_DOWN:
            outputGate = gate("down1$o");
            break;
        case Direction::ISL_UP:
            outputGate = gate("up1$o");
            break;
        case Direction::ISL_LEFT:
            outputGate = gate("left1$o");
            break;
        case Direction::ISL_RIGHT:
            outputGate = gate("right1$o");
            break;
        case Direction::ISL_DOWNLINK:
            outputGate = gate("groundLink1$o", routeInformation.gateIndex);
            break;
        default:
            error("Unexpected gate");
    }
    routeMessage(outputGate, msg);
}

void PacketHandlerRouting::receiveMessage(cMessage *msg) {
    send(msg, "queueOut");
}

void PacketHandlerRouting::routeMessage(cGate *gate, cMessage *msg) {
    if (gate->getTransmissionChannel()->isBusy()) {
        // EV << "SAT [" << satIndex << ", " << sequenceNumber << "]: Send down is busy." << endl;
        scheduleAt(gate->getTransmissionChannel()->getTransmissionFinishTime(), msg);
    } else {
        // EV << "SAT [" << satIndex << ", " << sequenceNumber << "]: Send down." << endl;
        send(msg->dup(), gate);
    }
}

bool PacketHandlerRouting::isExpired(Packet *pkt) {
    auto frame = pkt->removeAtFront<RoutingHeader>();
    int hops = frame->getNumHop();
    pkt->insertAtFront(frame);
    return hops > maxHops;
}

void PacketHandlerRouting::insertSatinRoute(Packet *pkt) {
    auto frame = pkt->removeAtFront<RoutingHeader>();
    int numHops = frame->getNumHop();
    frame->setNumHop(numHops + 1);
    frame->appendRoute(satIndex);
    frame->appendTimestamps(simTime());
    pkt->insertAtFront(frame);
}

}  // namespace satellite
}  // namespace flora
