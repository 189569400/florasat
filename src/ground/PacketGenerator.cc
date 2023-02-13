/*
 * PacketGenerator.cc
 *
 *  Created on: Feb 08, 2023
 *      Author: Robin Ohs
 */

#include "PacketGenerator.h"

namespace flora
{
    Define_Module(PacketGenerator);

    void PacketGenerator::initialize(int stage)
    {
        if (stage == 0)
        {
            groundStationId = getParentModule()->par("groundStationId");
            updateInterval = par("updateInterval");
            numGroundStations = getSystemModule()->getSubmoduleVectorSize("groundStation");
        }
        else if (stage == inet::INITSTAGE_APPLICATION_LAYER)
        {
            selfMsg = new cMessage("send_msg");
            scheduleUpdate();
        }
    }

    void PacketGenerator::scheduleUpdate()
    {
        cancelEvent(selfMsg);
        if (selfMsg != 0)
        {
            simtime_t nextUpdate = simTime() + updateInterval;
            scheduleAt(nextUpdate, selfMsg);
        }
    }

    void PacketGenerator::handleMessage(cMessage *msg)
    {
        if (msg->isSelfMessage())
        {
            handleSelfMessage(msg);
        }
        else
        {
            receiveMessage(msg);
        }
    }

    void PacketGenerator::handleSelfMessage(cMessage *msg)
    {
        auto newPacket = new Packet("DataFrame");
        auto payload = makeShared<RoutingFrame>();
        payload->setSequenceNumber(sentPackets);
        payload->setSourceGroundstation(groundStationId);
        int destination = getRandomNumber();
        payload->setDestinationGroundstation(destination);
        payload->setOriginTime(simTime());

        newPacket->insertAtFront(payload);

        std::stringstream ss;
        ss << "GS[" << groundStationId << "]: Send msg to " << destination << ".";
        EV << ss.str() << endl;

        send(newPacket, "satelliteLink$o");

        sentPackets = sentPackets + 1;
        scheduleUpdate();
    }

    int PacketGenerator::getRandomNumber()
    {
        int rn = intuniform(0, numGroundStations - 1);
        while (rn == groundStationId)
        {
            rn = intuniform(0, numGroundStations - 1);
        }
        return rn;
    }

    void PacketGenerator::receiveMessage(cMessage *msg)
    {
        auto pkt = check_and_cast<inet::Packet *>(msg);
        auto frame = pkt->removeAtFront<RoutingFrame>();

        int source = frame->getSourceGroundstation();
        int destination = frame->getDestinationGroundstation();
        int hops = frame->getNumHop();
        simtime_t latency = simTime() - frame->getOriginTime();

        std::stringstream ss;
        ss << "GS[" << destination << "]: Received msg from " << source << endl << " -> Hops:" << hops << " | Latency: " << latency * 1000 << "ms | Route: {";
        for (size_t i = 0; i < frame->getRouteArraySize(); i++)
        {
            int pathSat = frame->getRoute(i);
            ss << pathSat << ",";
        }
        ss << "}.";
        EV << ss.str() << endl;

        receivedPackets = receivedPackets + 1;
        delete msg;
    }

} // flora