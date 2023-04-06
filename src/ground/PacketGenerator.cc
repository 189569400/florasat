/*
 * PacketGenerator.cc
 *
 *  Created on: Feb 08, 2023
 *      Author: Robin Ohs
 */

#include "PacketGenerator.h"

namespace flora {

Define_Module(PacketGenerator);

void PacketGenerator::initialize(int stage) {
    if (stage == 0) {
        groundStationId = getParentModule()->par("groundStationId");
        numGroundStations = getSystemModule()->getSubmoduleVectorSize("groundStation");
    } else if (stage == inet::INITSTAGE_APPLICATION_LAYER) {
    }
}

void PacketGenerator::handleMessage(cMessage *msg) {
    if (msg->arrivedOn("satelliteLink")) {
        send(msg, "server$o");
    } else {
        // auto pkt = check_and_cast<inet::Packet *>(msg);
        // sendMessage(pkt);
    }
}

void PacketGenerator::sendMessage(inet::Packet *msg) {
    EV_INFO << msg << endl;
    auto newPacket = new Packet("DataFrame");
    auto payload = makeShared<RoutingFrame>();
    payload->setSequenceNumber(sentPackets);
    payload->setSourceGroundstation(groundStationId);
    int destination = core::utils::randomNumber(this, 0, numGroundStations - 1, groundStationId);
    payload->setDestinationGroundstation(destination);
    payload->setOriginTime(simTime());

    newPacket->insertAtFront(payload);

    // std::stringstream ss;
    // ss << "GS[" << groundStationId << "]: Send msg to " << destination << ".";
    // EV << ss.str() << endl;

    // for (size_t i = 0; i < 20; i++) {
    //     if (getParentModule()->gateHalf("satelliteLink", cGate::Type::OUTPUT, i)->isConnectedOutside()) {
    //         cGate *gate = gateHalf("satelliteLink", cGate::Type::OUTPUT, i);
    //         send(newPacket, gate);
    //         break;
    //     }
    // }
}

// int PacketGenerator::getRandomNumber()
// {
//     int rn = intuniform(0, numGroundStations - 1);
//     while (rn == groundStationId)
//     {
//         rn = intuniform(0, numGroundStations - 1);
//     }
//     return rn;
// }

// void PacketGenerator::receiveMessage(cMessage *msg)
// {
//     auto pkt = check_and_cast<inet::Packet *>(msg);
//     auto frame = pkt->removeAtFront<RoutingFrame>();

//     int destination = frame->getDestinationGroundstation();
//     frame->setReceptionTime(simTime());

//     if (groundStationId != destination)
//     {
//         metricsCollector->record_packet(metrics::PacketState::WRONG_DELIVERED, *frame.get());
//     }
//     else
//     {
//         metricsCollector->record_packet(metrics::PacketState::DELIVERED, *frame.get());
//     }
//     delete msg;
// }

}  // namespace flora