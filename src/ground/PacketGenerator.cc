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

            metricsCollector = check_and_cast<metrics::MetricsCollector *>(getSystemModule()->getSubmodule("metricsCollector"));
            if (metricsCollector == nullptr)
            {
                error("PacketGenerator::initialize(0): metricsCollector mullptr");
            }
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

        for (size_t i = 0; i < 20; i++)
        {
            if (getParentModule()->gateHalf("satelliteLink", cGate::Type::OUTPUT, i)->isConnectedOutside())
            {
                cGate *gate = gateHalf("satelliteLink", cGate::Type::OUTPUT, i);
                send(newPacket, gate);
                break;
            }
        }
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

        int destination = frame->getDestinationGroundstation();
        frame->setReceptionTime(simTime());

        if (groundStationId != destination)
        {
            metricsCollector->record_packet(metrics::PacketState::WRONG_DELIVERED, *frame.get());
        }
        else
        {
            metricsCollector->record_packet(metrics::PacketState::DELIVERED, *frame.get());
        }
        delete msg;
    }

} // flora