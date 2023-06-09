/*
 * PacketGenerator.cc
 *
 *  Created on: May 27, 2023
 *      Author: Sebastian Montoya
 */

#include "DtnPacketGenerator.h"

namespace flora {

Define_Module(DtnPacketGenerator);

void DtnPacketGenerator::initialize(int stage) {
    if (stage == INITSTAGE_LOCAL) {

        // Previous Code
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

void DtnPacketGenerator::parseBundlesNumber() {
    const char* bundlesNumberChar = par("bundlesNumber");
    cStringTokenizer bundlesNumberTokenizer(bundlesNumberChar, ",");
    while (bundlesNumberTokenizer.hasMoreTokens()) {
        bundlesNumber.push_back(atoi(bundlesNumberTokenizer.nextToken()));
    }
}

void DtnPacketGenerator::parseDestinationsEid(){
    const char *destinationEidChar = par("destinationEid");
    cStringTokenizer destinationEidTokenizer(destinationEidChar, ",");
    while (destinationEidTokenizer.hasMoreTokens())
    {
        std::string destinationEidStr = destinationEidTokenizer.nextToken();
        int destinationEid = stoi(destinationEidStr);
        ASSERT(destinationEid > this->getParentModule()->getVectorSize());
        destinationsEid.push_back(destinationEid);
    }
}

void DtnPacketGenerator::parseSizes(){
    const char *sizeChar = par("size");
    cStringTokenizer sizeTokenizer(sizeChar, ",");
    while (sizeTokenizer.hasMoreTokens())
        sizes.push_back(atoi(sizeTokenizer.nextToken()));
}

void DtnPacketGenerator::parseStarts(){
    const char *startChar = par("start");
    cStringTokenizer startTokenizer(startChar, ",");
    while (startTokenizer.hasMoreTokens())
        starts.push_back(atof(startTokenizer.nextToken()));
}

void DtnPacketGenerator::finish() {
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

void DtnPacketGenerator::handleMessage(cMessage *msg) {
    auto pkt = check_and_cast<inet::Packet *>(msg);
    if (msg->arrivedOn("satelliteLink$i")) {
        decapsulate(pkt);
        send(pkt, "transportOut");
    } else if (msg->arrivedOn("transportIn")) {
        auto frame = pkt->peekAtFront<DtnTransportHeader>();

        EV << "Bundles Number: " << frame->getBundlesNumber()
           << "Bundles Destination Id: " << frame->getDestinationEid()
           << "Bundle Size: " << frame->getSize()
           << "Bundle Interval: " << frame->getInterval()
           << "Bundle TTL: " << frame->getTtl() << endl;

//        int dstGs = routingTable->getGroundstationFromAddress(L3Address(frame->getDstIpAddress()));
//
//        auto satPair = routingModule->calculateFirstAndLastSatellite(groundStationId, dstGs);
//        auto firstSat = satPair.first;
//        auto lastSat = satPair.second;
//
//        encapsulate(pkt, dstGs, firstSat, lastSat);
//
//        int gateIndex = topologycontrol->getGroundstationSatConnection(groundStationId, firstSat).gsGateIndex;
//        send(pkt, "satelliteLink$o", gateIndex);
    } else {
        error("Unexpected gate");
    }
}

void DtnPacketGenerator::encapsulate(Packet *packet, int dstGs, int firstSat, int lastSat) {
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

void DtnPacketGenerator::decapsulate(Packet *packet) {
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

std::vector<int> DtnPacketGenerator::getBundlesNumberVec()
{
    return this->bundlesNumber;
}

std::vector<int> DtnPacketGenerator::getDestinationEidVec()
{
    return this->destinationsEid;
}

std::vector<int> DtnPacketGenerator::getSizeVec()
{
    return this->sizes;
}

std::vector<double> DtnPacketGenerator::getStartVec()
{
    return this->starts;
}

}  // namespace flora
