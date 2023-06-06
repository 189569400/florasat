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
    // ActivePacketSinkBase::initialize(stage);
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
    // else if (stage == INITSTAGE_QUEUEING) {
    //     if (provider->canPullSomePacket(inputGate->getPathStartGate())) {
    //         collectPacket();
    //     }
    // }
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
        auto frame = pkt->peekAtFront<Ipv4Header>();
        EV << "RCVD: " << frame->getSrcAddress() << "->" << frame->getDestAddress() << endl;
        // auto frame = pkt->peekAtFront<TransportHeader>();
        int dstGs = routingTable->getGroundstationFromAddress(L3Address(frame->getDestAddress()));

        EV << "Send to " << dstGs << endl;

        auto satPair = routingModule->calculateFirstAndLastSatellite(groundStationId, dstGs);
        auto firstSat = satPair.first;
        auto lastSat = satPair.second;
        int gateIndex = topologycontrol->getGroundstationSatConnection(groundStationId, firstSat).gsGateIndex;

        cGate *satLink = gate("satelliteLink$o", gateIndex);
        if (satLink->getTransmissionChannel()->isBusy()) {
            auto &tag = pkt->addTagIfAbsent<GroundLinkTag>();
            tag->setDstGs(dstGs);
            tag->setFirstSat(firstSat);
            tag->setLastSat(lastSat);
            tag->setGateIndex(gateIndex);
            scheduleAt(satLink->getTransmissionChannel()->getTransmissionFinishTime(), pkt);
        } else {
            encapsulate(pkt, dstGs, firstSat, lastSat);
            send(pkt, satLink);
        }
    } else if (msg->isSelfMessage()) {
        auto &tag = pkt->getTag<GroundLinkTag>();
        auto gateIndex = tag->getGateIndex();
        cGate *satLink = gate("satelliteLink$o", gateIndex);
        if (satLink->getTransmissionChannel()->isBusy()) {
            scheduleAt(satLink->getTransmissionChannel()->getTransmissionFinishTime(), pkt);
        } else if (!satLink->isConnectedOutside()) {
            send(pkt, "transportIn");
        } else {
            auto dstGs = tag->getDstGs();
            auto firstSat = tag->getFirstSat();
            auto lastSat = tag->getLastSat();
            pkt->removeTag<GroundLinkTag>();
            encapsulate(pkt, dstGs, firstSat, lastSat);
            send(pkt, satLink);
        }
    } else {
        error("Unexpected gate");
    }
}

void PacketGenerator::encapsulate(Packet *packet, int dstGs, int firstSat, int lastSat) {
    auto header = makeShared<RoutingHeader>();
    header->setTos(ToS::LOW);
    header->setType(Type::G2G);
    header->setIdent(numSent);
    header->setTTL(51);
    header->setSrcGs(groundStationId);
    header->setDstGs(dstGs);
    header->setSrcSat(firstSat);
    header->setDstSat(lastSat);
    header->setChunkLength(B(20));
    header->setTotalLength(packet->getTotalLength() + B(20));

    packet->insertAtFront(header);

    numSent++;
    sentBytes += header->getTotalLength();
    emit(packetSentSignal, packet);
}

void PacketGenerator::decapsulate(Packet *packet) {
    auto header = packet->popAtFront<RoutingHeader>();
    if (header->getDstGs() != groundStationId) {
        error("Packet was routed to wrong destination.");
    }

    // hopCountVector.record(header->getNumHop());
    // hopCountStats.collect(header->getNumHop());

    auto rcvdBytes = header->getTotalLength();

    EV_DEBUG << rcvdBytes << " vs. " << packet->getDataLength() << endl;

    VALIDATE(packet->getDataLength() == rcvdBytes - B(20));  // if the packet is correct

    numReceived++;
    receivedBytes += rcvdBytes;
    emit(packetReceivedSignal, packet);
}

}  // namespace flora
