/*
 * DsprRouting.cc
 *
 * Created on: Feb 04, 2023
 *     Author: Robin Ohs
 */

#include "DsprRouting.h"

namespace flora {
namespace routing {

Define_Module(DsprRouting);

void DsprRouting::initialize(int stage) {
    RoutingBase::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {
        broadcastId = 0;
    }
}

void DsprRouting::handleTopologyChange() {
    Enter_Method("handleTopologyChange");
    // generate cost matrix for topology
    auto costMatrix = core::dspa::buildShortestPathCostMatrix(topologyControl->getSatellites());

    for (size_t sId = 0; sId < topologyControl->getNumberOfSatellites(); sId++) {
        auto sat = topologyControl->getSatellite(sId);
        auto forwardingTable = static_cast<ForwardingTable *>(sat->getSubmodule("forwardingTable"));
        forwardingTable->clearRoutes();
        // run dijkstra to get shortest path to all satellites
        core::dspa::DijkstraResult result = core::dspa::runDijkstra(sId, costMatrix);
        for (size_t gsId = 0; gsId < topologyControl->getNumberOfGroundstations(); gsId++) {
            auto gs = topologyControl->getGroundstationInfo(gsId);
            // get lastSatellite with shortest distance + groundlink distance
            int shortestDistanceSat = -1;
            int shortestDistance = 999999999;
            for (auto pLastSat : gs->getSatellites()) {
                // add distance between sat and gs for selecting optimal last satellites
                double distance = result.distances[pLastSat] + gs->getDistance(*sat);
                if (shortestDistance > distance) {
                    shortestDistance = distance;
                    shortestDistanceSat = pLastSat;
                }
            }

            ASSERT(shortestDistanceSat != -1);

            // update forwarding table accordingly
            if (shortestDistanceSat == sId) {
                forwardingTable->setRoute(gsId, isldirection::GROUNDLINK);
            } else {
                int nextSatId = core::dspa::reconstructPath(sId, shortestDistanceSat, result.prev)[1];
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
}

ISLDirection DsprRouting::routePacket(inet::Ptr<const RoutingHeader> rh, SatelliteRouting *callerSat) {
    Enter_Method("routePacket");
    auto forwardingTable = static_cast<ForwardingTable *>(callerSat->getSubmodule("forwardingTable"));
    return forwardingTable->getNextHop(rh->getDstGs());
}

void DsprRouting::handleMessage(inet::Packet *pkt, SatelliteRouting *callerSat) {
    Enter_Method("handleMessage");

    EV << "Handle: " << pkt << endl;
}

Packet *DsprRouting::handlePacketDrop(Packet *pkt, SatelliteRouting *callerSat, PacketDropReason reason) {
    Enter_Method("handlePacketDrop");

    if (reason == PacketDropReason::INTERFACE_DOWN) {
        // create header for pkt
        auto data = makeShared<ByteCountChunk>(B(1000));
        auto dataPacket = new Packet("IF DOWN", data);  // create new packet with data
        auto header = makeShared<RoutingHeader>();
        header->setTos(ToS::HIGH);
        header->setType(Type::SBroadcast);
        header->setIdent(broadcastId);
        broadcastId++;
        header->setTTL(1);
        header->setSrcGs(-1);
        header->setDstGs(-1);
        header->setSrcSat(callerSat->getId());
        header->setDstSat(-1);
        header->setChunkLength(B(20));
        header->setTotalLength(dataPacket->getTotalLength() + B(20));
        dataPacket->insertAtFront(header);
        return dataPacket;
    }
    return nullptr;
}

}  // namespace routing
}  // namespace flora
