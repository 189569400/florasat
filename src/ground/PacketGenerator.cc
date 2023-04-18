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
    if (stage == INITSTAGE_LOCAL) {
        groundStationId = getParentModule()->par("groundStationId");
        numGroundStations = getSystemModule()->getSubmoduleVectorSize("groundStation");

        numSent = 0;
        numReceived = 0;
        sentBytes = B(0);
        receivedBytes = B(0);
        WATCH(numSent);
        WATCH(numReceived);
        WATCH(sentBytes);
        WATCH(receivedBytes);
    }
}

void PacketGenerator::handleMessage(cMessage *msg) {
    auto pkt = check_and_cast<inet::Packet *>(msg);
    if (msg->arrivedOn("satelliteLink$i")) {
        decapsulate(pkt);
        send(pkt, "transportOut");
    } else {
        auto destinationId = 0;
        encapsulate(pkt, destinationId);
        for (size_t i = 0; i < 40; i++) {
            if (getParentModule()->gateHalf("satelliteLink", cGate::Type::OUTPUT, i)->isConnectedOutside()) {
                cGate *gate = gateHalf("satelliteLink", cGate::Type::OUTPUT, i);
                send(pkt, gate);
                break;
            }
        }
    }
}

void PacketGenerator::encapsulate(inet::Packet *packet, int destinationId) {
    auto header = makeShared<RoutingHeader>();
    header->setChunkLength(B(8));
    header->setSequenceNumber(sentPackets);
    header->setLength(packet->getTotalLength());
    header->setSourceGroundstation(groundStationId);
    header->setDestinationGroundstation(destinationId);
    header->setOriginTime(simTime());
    packet->insertAtFront(header);

    numSent++;
    sentBytes += (header->getChunkLength() + header->getLength());
    emit(packetSentSignal, packet);
}

void PacketGenerator::decapsulate(inet::Packet *packet) {
    auto header = packet->popAtFront<RoutingHeader>();
    auto lengthField = header->getLength();
    auto rcvdBytes = header->getChunkLength() + lengthField;

    assert(packet->getDataLength() == lengthField);  // if the packet is correct

    numReceived++;
    receivedBytes += rcvdBytes;
    emit(packetReceivedSignal, packet);
    //EV << "RCVD(" << rcvdBytes << "):" << header->getSourceGroundstation() << " -> " << header->getDestinationGroundstation() << endl;
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
//     auto frame = pkt->removeAtFront<RoutingHeader>();

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