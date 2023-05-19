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
        groundStationId = getParentModule()->getIndex();
        numGroundStations = getSystemModule()->getSubmoduleVectorSize("groundStation");
        topologycontrol = check_and_cast<topologycontrol::TopologyControlBase *>(getSystemModule()->getSubmodule("topologyControl"));
        routingTable = check_and_cast<networklayer::ConstellationRoutingTable *>(getSystemModule()->getSubmodule("constellationRoutingTable"));
        routingModule = check_and_cast<routing::RoutingBase *>(getSystemModule()->getSubmodule("routing"));

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

void PacketGenerator::finish() {
    // This function is called by OMNeT++ at the end of the simulation.
    EV << "Sent:     " << numSent << endl;
    EV << "Received: " << numReceived << endl;
    EV << "Hop count, min:    " << hopCountStats.getMin() << endl;
    EV << "Hop count, max:    " << hopCountStats.getMax() << endl;
    EV << "Hop count, mean:   " << hopCountStats.getMean() << endl;
    EV << "Hop count, stddev: " << hopCountStats.getStddev() << endl;

    recordScalar("#sent", numSent);
    recordScalar("#received", numReceived);

    hopCountStats.recordAs("hop count");
}

void PacketGenerator::handleMessage(cMessage *msg) {
    auto pkt = check_and_cast<inet::Packet *>(msg);
    if (msg->arrivedOn("satelliteLink$i")) {
        decapsulate(pkt);
        send(pkt, "transportOut");
    } else if (pkt->arrivedOn("transportIn")) {
        auto frame = pkt->peekAtFront<TransportHeader>();
        int dstGs = routingTable->getGroundstationFromAddress(L3Address(frame->getDstIpAddress()));

        auto satPair = routingModule->calculateFirstAndLastSatellite(groundStationId, dstGs);
        auto firstSat = satPair.first;
        auto lastSat = satPair.second;

        encapsulate(pkt, dstGs, firstSat, lastSat);

        int gateIndex = topologycontrol->getGroundstationSatConnection(groundStationId, firstSat).gsGateIndex;
        send(pkt, "satelliteLink$o", gateIndex);
    } else {
        error("Unexpected gate");
    }
}

void PacketGenerator::encapsulate(Packet *packet, int dstGs, int firstSat, int lastSat) {
    auto header = makeShared<RoutingHeader>();
    header->setChunkLength(B(8));
    header->setSequenceNumber(sentPackets);
    header->setLength(packet->getTotalLength());
    header->setSourceGroundstation(groundStationId);
    header->setDestinationGroundstation(dstGs);
    header->setFirstSatellite(firstSat);
    header->setLastSatellite(lastSat);
    header->setOriginTime(simTime());
    packet->insertAtFront(header);

    numSent++;
    sentBytes += (header->getChunkLength() + header->getLength());
    emit(packetSentSignal, packet);
}

void PacketGenerator::decapsulate(Packet *packet) {
    auto header = packet->popAtFront<RoutingHeader>();
    if (header->getDestinationGroundstation() != groundStationId) {
        error("Packet was routed to wrong destination.");
    }

    hopCountVector.record(header->getNumHop());
    hopCountStats.collect(header->getNumHop());

    auto lengthField = header->getLength();
    auto rcvdBytes = header->getChunkLength() + lengthField;

    VALIDATE(packet->getDataLength() == lengthField);  // if the packet is correct

    numReceived++;
    receivedBytes += rcvdBytes;
    emit(packetReceivedSignal, packet);
}

}  // namespace flora