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
        numSatellites = getSystemModule()->getSubmoduleVectorSize("loRaGW");
        topologycontrol = check_and_cast<topologycontrol::TopologyControlBase *>(getSystemModule()->getSubmodule("topologyControl"));
        routingTable = check_and_cast<networklayer::ConstellationRoutingTable *>(getSystemModule()->getSubmodule("constellationRoutingTable"));
        contactPlan = check_and_cast<ContactPlan *>(getSystemModule()->getSubmodule("contactPlan"));

        // TODO: Check what global contact is
        routingModule = new routing::RoutingCgrModelRev17(groundStationId + 1, numGroundStations + numSatellites, &sdr_, contactPlan, contactPlan, "none", true);

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

    ///////////////////////////////////////////
    // New Bundle
    ///////////////////////////////////////////
    if (msg->getKind() == BUNDLE || msg->getKind() == BUNDLE_CUSTODY_REPORT) {
        DtnRoutingHeader *frame = const_cast<DtnRoutingHeader *>(pkt->peekAtFront<DtnRoutingHeader>().get());
        EV << "Bundle Id: " << frame->getBundleId()
           << " Source Id: " << frame->getSourceEid() << endl;
        dispatchBundle(frame);
    }
    ///////////////////////////////////////////
    // Forwarding Stage
    ///////////////////////////////////////////
    else if (msg->getKind() == FORWARDING_MSG_START )
    {
        EV << "Forwarding Message Start" << endl;
    } else if (msg->arrivedOn("satelliteLink$i")) {
        decapsulate(pkt);
        send(pkt, "transportOut");
    } else if (msg->arrivedOn("transportIn")) {

        if (msg->getKind() == TRANSPORT_HEADER) {
            auto frame = pkt->peekAtFront<DtnTransportHeader>();

            EV << "Bundles Number: " << frame->getBundlesNumber()
               << " Bundles Destination Id: " << frame->getDestinationEid()
               << " Bundle Size: " << frame->getSize()
               << " Bundle Interval: " << frame->getInterval()
               << " Bundle TTL: " << frame->getTtl()
               << " GroundStation Id: " << groundStationId << endl;

            const auto routingHeader = makeShared<DtnRoutingHeader>();
            routingHeader->setBundleId(routingHeader->getBundleId());
            routingHeader->setChunkLength(frame->getChunkLength());

            // Bundle Fields (Set by source Node)
            routingHeader->setSourceEid(groundStationId + 1);
            routingHeader->setDestinationEid(frame->getDestinationEid());
            routingHeader->setReturnToSender(par("returnToSender"));
            routingHeader->setCritical(par("critical"));
            routingHeader->setCustodyTransferRequested(par("custodyTransfer"));
            routingHeader->setTtl(frame->getTtl());
            routingHeader->setCreationTimestamp(simTime());
            routingHeader->setQos(2);
            routingHeader->setBundleIsCustodyReport(false);

            // Bundle meta-data (set by intermediate nodes)
            routingHeader->setHopCount(0);
            routingHeader->setNextHopEid(0);
            routingHeader->setSenderEid(groundStationId + 1); //TODO: check this logic
            routingHeader->setCustodianEid(groundStationId + 1);
            routingHeader->getVisitedNodesForUpdate().clear();
            routing::CgrRoute emptyRoute;
            emptyRoute.nextHop = EMPTY_ROUTE;
            routingHeader->setCgrRoute(emptyRoute);
            pkt->insertAtFront(routingHeader);
            pkt->setKind(BUNDLE);
            scheduleAt(simTime(), pkt);
        }
    }
}

void DtnPacketGenerator::dispatchBundle(DtnRoutingHeader *bundle) {
    EV << "Starting dispatchBundle: " << endl;
    if (groundStationId + 1 == bundle->getDestinationEid()) {
        EV << "dispatchBundle: Bundle arrived to destination" << endl;
        // TODO: Arrived to destination
    } else {
        EV << "dispatchBundle: Bundle in transit" << endl;
        // TODO: Bundle in transit
        if (bundle->getCustodyTransferRequested()) {
            dispatchBundle(custodyModel_.bundleWithCustodyRequestedArrived(bundle));
        }
        routingModule->msgToOtherArrive(bundle, simTime().dbl());
       /* emit(routeCgrDijkstraCalls, routingModule->getDijkstraCalls());
        emit(routeCgrDijkstraLoops, routingModule>getDijkstraLoops());
        emit(routeCgrRouteTableEntriesCreated, routingModule->getRouteTableEntriesCreated());
        emit(routeCgrRouteTableEntriesExplored, routingModule->getRouteTableEntriesExplored());*/
    }
    /*emit(sdrBundleStored, sdr_.getBundlesCountInSdr());
    emit(sdrBytesStored, sdr_.getBytesStoredInSdr());*/

    // Wake-up sleeping forwarding threads
    refreshForwarding();
}

void DtnPacketGenerator::refreshForwarding(){
    // Check all on-going forwardingMsgs threads
    // (contacts) and wake up those not scheduled.
    std::map<int, ForwardingMsgStart*>::iterator it;
    for (it = forwardingMsgs_.begin(); it != forwardingMsgs_.end(); ++it)
    {
        ForwardingMsgStart *forwardingMsg = it->second;
        int cid = forwardingMsg->getContactId();
        if (!sdr_.isBundleForContact(cid))
            //notify routing protocol that it has messages to send and contacts for routing
            routingModule->refreshForwarding(contactPlan->getContactById(cid));
        if (!forwardingMsg->isScheduled())
        {
            scheduleAt(simTime(), forwardingMsg);
        }
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
