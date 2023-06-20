/*
 * DDRARouting.cc
 *
 * Created on: Feb 04, 2023
 *     Author: Robin Ohs
 */

#include "DDRARouting.h"

namespace flora {
namespace routing {

Define_Module(DDRARouting);

void DDRARouting::initialize(int stage) {
    ClockUserModuleMixin::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {
        broadcastId = 0;
        queueThreshold = par("queueThreshold");
        timeSliceInterval = par("timeSliceLength").doubleValue();
        timeSliceTimer = new ClockEvent("TimeSliceTimer");
        scheduleClockEventAfter(timeSliceInterval, timeSliceTimer);
    } else if (stage == INITSTAGE_ROUTING_PROTOCOLS) {
        // init state map
        int constellationSize = topologyControl->getNumberOfSatellites();
        state = std::unordered_map<int, SatelliteState>();
        state.reserve(constellationSize);
        for (size_t i = 0; i < constellationSize; i++) {
            state[i] = SatelliteState{};
        }
    }
}

void DDRARouting::handleMessage(cMessage *msg) {
    if (msg == timeSliceTimer) {
        int constellationSize = topologyControl->getNumberOfSatellites();
        auto costMatrix = core::dspa::buildShortestPathCostMatrix(constellationSize, topologyControl->getSatellites());
        for (size_t satId = 0; satId < constellationSize; satId++) {
            SatelliteState &satState = state.at(satId);
            satState.congestedSats.clear();
            satState.congestedMsgSent = false;
            calculateRoutingTable(costMatrix, satId);
        }
        scheduleClockEventAfter(timeSliceInterval, timeSliceTimer);
    }
}

void DDRARouting::handleTopologyChange() {
    Enter_Method("handleTopologyChange");
    int constellationSize = topologyControl->getNumberOfSatellites();
    auto costMatrix = core::dspa::buildShortestPathCostMatrix(constellationSize, topologyControl->getSatellites());

    for (size_t satId = 0; satId < constellationSize; satId++) {
        calculateRoutingTable(costMatrix, satId);
    }
}

void DDRARouting::calculateRoutingTable(std::vector<std::vector<int>> &costMatrix, int satId) {
    auto sat = topologyControl->getSatellite(satId);
    auto forwardingTable = static_cast<ForwardingTable *>(sat->getSubmodule("forwardingTable"));
    forwardingTable->clearRoutes();
    // run dijkstra to get shortest path to all satellites
    core::dspa::DijkstraResultFull dijkstraResultFull = core::dspa::runDijkstraFull(costMatrix, satId);
    for (size_t gsId = 0; gsId < topologyControl->getNumberOfGroundstations(); gsId++) {
        auto gs = topologyControl->getGroundstationInfo(gsId);

        // get lastSatellite with shortest distance
        int nearestSat = core::dspa::getNearestId(dijkstraResultFull, gs->getSatellites());

        // update forwarding table accordingly
        if (nearestSat == satId) {
            forwardingTable->setRoute(gsId, isldirection::GROUNDLINK);
        } else {
            int nextSatId = core::dspa::reconstructPath(dijkstraResultFull, nearestSat)[1];
            if (sat->hasLeftSat() && sat->getLeftSatId() == nextSatId) {
                forwardingTable->setRoute(gsId, isldirection::LEFT);
            } else if (sat->hasUpSat() && sat->getUpSatId() == nextSatId) {
                forwardingTable->setRoute(gsId, isldirection::UP);
            } else if (sat->hasRightSat() && sat->getRightSatId() == nextSatId) {
                forwardingTable->setRoute(gsId, isldirection::RIGHT);
            } else if (sat->hasDownSat() && sat->getDownSatId() == nextSatId) {
                forwardingTable->setRoute(gsId, isldirection::DOWN);
            } else {
                error("Path not found.");
            }
        }
    }
#ifndef NDEBUG
    forwardingTable->printForwardingTable();
#endif
}

ISLDirection DDRARouting::routePacket(inet::Ptr<const RoutingHeader> rh, SatelliteRouting *callerSat) {
    Enter_Method("routePacket");
    auto forwardingTable = static_cast<ForwardingTable *>(callerSat->getSubmodule("forwardingTable"));
    return forwardingTable->getNextHop(rh->getDstGs());
}

void DDRARouting::handleMessage(inet::Packet *pkt, SatelliteRouting *callerSat) {
    Enter_Method("handleMessage");
    if (strcmp(pkt->getName(), "DDRA-CONGESTED") == 0) {
        // update state of satellite
        int satId = callerSat->getId();
        SatelliteState &satState = state.at(satId);
        int congestedSatId = pkt->peekAtFront<RoutingHeader>()->getSrcSat();
        EV << "Satellite " << satId << " received congested notification from " << congestedSatId << endl;
        satState.congestedSats.emplace(congestedSatId);
        EV << " -> " << satId << "'s known congested sats: " << flora::core::utils::set::toString(satState.congestedSats.begin(), satState.congestedSats.end()) << endl;

        // update routing table
        int constellationSize = topologyControl->getNumberOfSatellites();
        auto costMatrix = core::dspa::buildShortestPathCostMatrix(constellationSize, topologyControl->getSatellites());
        for (int id : satState.congestedSats) {
            costMatrix[satId][id] = INT_MAX;
        }
        calculateRoutingTable(costMatrix, satId);
    } else {
        error("unhandled message");
    }
}

inet::Packet *DDRARouting::handlePacketDrop(const Packet *pkt, SatelliteRouting *callerSat, PacketDropReason reason) {
    Enter_Method("handlePacketDrop");
    return nullptr;
}

inet::Packet *DDRARouting::handleQueueSize(SatelliteRouting *callerSat, int queueSize, int maxQueueSize) {
    int satId = callerSat->getId();
    SatelliteState &satState = state.at(satId);

    if (queueSize > queueThreshold && !satState.congestedMsgSent) {
        satState.congestedMsgSent = true;
        inet::Packet *transportPacket = createTransportPacket(satId);
        return transportPacket;
    } else if (queueSize < queueThreshold && satState.congestedMsgSent) {
        satState.congestedMsgSent = false;
    }
    return nullptr;
}

inet::Packet *DDRARouting::createTransportPacket(int srcId) {
    // create header for pkt
    auto data = makeShared<ByteCountChunk>(B(1000));
    auto dataPacket = new Packet("DDRA-CONGESTED", data);  // create new packet with data
    auto header = makeShared<RoutingHeader>();
    header->setTos(ToS::HIGH);
    header->setType(Type::SBroadcast);
    header->setIdent(broadcastId);
    broadcastId++;
    header->setTTL(1);
    header->setSrcGs(-1);
    header->setDstGs(-1);
    header->setSrcSat(srcId);
    header->setDstSat(-1);
    header->setChunkLength(B(20));
    header->setTotalLength(dataPacket->getTotalLength() + B(20));
    dataPacket->insertAtFront(header);
    return dataPacket;
}

}  // namespace routing
}  // namespace flora
