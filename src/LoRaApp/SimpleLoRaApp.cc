//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#include "SimpleLoRaApp.h"
#include "inet/common/packet/Packet.h"
#include "inet/mobility/static/StationaryMobility.h"
#include "LoRa/LoRaTagInfo_m.h"
#include "mobility/UniformGroundMobility.h"

namespace flora {

Define_Module(SimpleLoRaApp);

void SimpleLoRaApp::initialize(int stage)
{
    cSimpleModule::initialize(stage);

    if (stage == INITSTAGE_APPLICATION_LAYER)
    {
        bool isOperational;
        NodeStatus *nodeStatus = dynamic_cast<NodeStatus*>(findContainingNode(
                this)->getSubmodule("status"));
        isOperational = (!nodeStatus) || nodeStatus->getState() == NodeStatus::UP;
        if (!isOperational)
            throw cRuntimeError("This module doesn't support starting in node DOWN state");

        // Parameters
        timeToFirstPacket = par("timeToFirstPacket");
        timeToNextPacket = par("timeToNextPacket");
        ackTimeout = par("ackTimeout"); //acknowledgment timeout
        timer = par("timer"); //timer used for acknowledgment implementation

        // Messages
        endAckTime = new cMessage("acknowledgment Timeout"); //signal to notice about end of ACK time
        sendUplink = new cMessage("sendMeasurements");

        synchronizer = new cMessage("Ask for device local time"); //signal for future use
        joining = new cMessage("Try to reach GW"); //signal for future use
        joiningAns = new cMessage("Can't reach GW ? .."); //signal for future use

        // Schedule first packet
        scheduleAt(simTime() + timeToFirstPacket, sendUplink);
        timer = simTime() + timeToFirstPacket;

        // Schedule first packet ack timeout
        if (par("usingAck").boolValue())
            scheduleAt(simTime() + timeToFirstPacket + ackTimeout, endAckTime);

        // Initialize  metrics
        receivedAckPackets = 0;
        sentPackets = 0;
        receivedADRCommands = 0;

        // Initialize parameters
        numberOfPacketsToSend = par("numberOfPacketsToSend");
        loRaTP = par("initialLoRaTP").doubleValue();
        loRaCF = units::values::Hz(par("initialLoRaCF").doubleValue());
        loRaSF = par("initialLoRaSF");
        loRaBW = inet::units::values::Hz(par("initialLoRaBW").doubleValue());
        loRaCR = par("initialLoRaCR");
        loRaUseHeader = par("initialUseHeader");
        pingSlot = par("initialPingSlot");
        evaluateADRinNode = par("evaluateADRinNode");

        // Metrics
        LoRa_AppPacketSent = registerSignal("LoRa_AppPacketSent");
        sfVector.setName("SF Vector");
        tpVector.setName("TP Vector");
        WATCH(receivedAck);
    }
}


void SimpleLoRaApp::finish()
{
    cModule *loRaNodeModule = getContainingNode(this);

    if (UniformGroundMobility *uniformMobility = dynamic_cast<UniformGroundMobility*>(loRaNodeModule->getSubmodule("mobility")))
    {
        recordScalar("Longitude", uniformMobility->getLongitude());
        recordScalar("Latitude", uniformMobility->getLatitude());
    }
    else
    {
        StationaryMobility *mobility = check_and_cast<StationaryMobility*>(loRaNodeModule->getSubmodule("mobility"));
        Coord coord = mobility->getCurrentPosition();
        recordScalar("positionX", coord.x);
        recordScalar("positionY", coord.y);
    }

    recordScalar("receivedAck", receivedAck);
    recordScalar("finalTP", loRaTP);
    recordScalar("finalSF", loRaSF);
    recordScalar("sentPackets", sentPackets);
    recordScalar("receivedAckPackets", receivedAckPackets);
    recordScalar("receivedADRCommands", receivedADRCommands);
}

void SimpleLoRaApp::handleMessage(cMessage *msg)
{
    // Either ack timer expired, or send measurement
    if (msg->isSelfMessage())
    {
        // Ack timer expired, No Ack received
        if (msg == endAckTime)
        {
            receivedAck = false;

            // More packets to send
            if (numberOfPacketsToSend == 0 || sentPackets < numberOfPacketsToSend)
            {
                // Schedule next packet
                sendUplink = new cMessage("sendMeasurements");
                scheduleAt(simTime() + timeToNextPacket - ackTimeout,
                        sendUplink);

                // Schedule next packet ack timeout
                endAckTime = new cMessage("Ack Timeout");
                scheduleAt(simTime() + timeToNextPacket, endAckTime);
                timer = timeToNextPacket + timer;
            }
        }

        // New packet to send
        if (msg == sendUplink)
        {
            delete msg;

            // Send new packet
            sendUplinkPacket();
            sentPackets++;

            // More packets to send
            if (numberOfPacketsToSend == 0 || sentPackets < numberOfPacketsToSend)
            {
                // Schedule next packet
                sendUplink = new cMessage("sendMeasurements");
                scheduleAt(simTime() + timeToNextPacket, sendUplink);

                // Schedule next packet ack timeout
                if (par("usingAck").boolValue())
                {
                    endAckTime = new cMessage("Ack Timeout");
                    scheduleAt(simTime() + timeToNextPacket, endAckTime);
                    timer = timeToNextPacket + timer;
                }
            }
        }
    }

    // Otherwise it is a message from outside (lower layer)
    else
    {
        handleMessageFromLowerLayer(msg);
        delete msg;
    }
}

void SimpleLoRaApp::handleMessageFromLowerLayer(cMessage *msg)
{
    auto pkt = check_and_cast<Packet*>(msg);
    const auto &packet = pkt->peekAtFront<LoRaAppPacket>();

    // Received ACK
    if (packet->getMsgType() == ACK)
    {
        cancelEvent(endAckTime);
        receivedAckPackets++;
        receivedAck = true;

        // More packets to send
        if (numberOfPacketsToSend == 0 || sentPackets < numberOfPacketsToSend)
        {
            // Schedule next packet
            sendUplink = new cMessage("sendMeasurements");
            scheduleAt(timer + timeToNextPacket, sendUplink);

            // Schedule next packet ack timeout
            endAckTime = new cMessage("Ack Timeout");
            scheduleAt(timer + timeToNextPacket + ackTimeout, endAckTime);
            timer = timeToNextPacket + timer;
        }

        ADR_ACK_CNT = 0;
    }

    // Received beacon
    if (packet->getMsgType() == Beacon)
    {
        //receive and set the pingSlot value to the value read from the GW beacon message
        pingSlot = packet->getOptions().getPingSlot();
        EV << pingSlot << endl;
    }

    // Received TxConfig
    if (packet->getMsgType() == TXCONFIG)
    {
        ADR_ACK_CNT = 0;
        if (evaluateADRinNode)
        {
            if (simTime() >= getSimulation()->getWarmupPeriod())
                receivedADRCommands++;

            if (packet->getOptions().getLoRaTP() != -1)
                loRaTP = packet->getOptions().getLoRaTP();

            if (packet->getOptions().getLoRaSF() != -1)
                loRaSF = packet->getOptions().getLoRaSF();

            EV << "New TP " << loRaTP << endl;
            EV << "New SF " << loRaSF << endl;
        }
    }
}

bool SimpleLoRaApp::handleOperationStage(LifecycleOperation *operation, IDoneCallback *doneCallback)
{
    Enter_Method_Silent();
    throw cRuntimeError("Unsupported lifecycle operation '%s'", operation->getClassName());
    return true;
}

void SimpleLoRaApp::sendUplinkPacket()
{
    auto uplinkPacket = new Packet("DataFrame");
    uplinkPacket->setKind(DATA);

    auto payload = makeShared<LoRaAppPacket>();
    payload->setChunkLength(B(par("payloadSize").intValue()));

    lastSentMeasurement = rand();
    payload->setSampleMeasurement(lastSentMeasurement);

    if (evaluateADRinNode && sendNextPacketWithADRACKReq)
    {
        auto opt = payload->getOptions();
        opt.setADRACKReq(true);
        payload->setOptions(opt);
        //request->getOptions().setADRACKReq(true);
        sendNextPacketWithADRACKReq = false;
    }

    auto loraTag = uplinkPacket->addTagIfAbsent<LoRaTag>();
    loraTag->setBandwidth(loRaBW);
    loraTag->setCenterFrequency(loRaCF);
    loraTag->setSpreadFactor(loRaSF);
    loraTag->setCodeRendundance(loRaCR);
    loraTag->setPower(mW(math::dBmW2mW(loRaTP)));

    sfVector.record(loRaSF);
    tpVector.record(loRaTP);

    uplinkPacket->insertAtBack(payload);

    // Send packet
    send(uplinkPacket, "appOut");

    // Evaluate ADR
    if (evaluateADRinNode && receivedAck == false)
    {
        ADR_ACK_CNT++;
        EV << ADR_ACK_CNT << endl;
        if (ADR_ACK_CNT == ADR_ACK_LIMIT)
            sendNextPacketWithADRACKReq = true;

        if (ADR_ACK_CNT >= ADR_ACK_LIMIT + ADR_ACK_DELAY)
        {
            ADR_ACK_CNT = 0;
            increaseSFIfPossible();
            EV << "i'm working on the ADRNode " << endl;
            EV << loRaSF << endl;
            EV << loRaTP << endl;
        }
    }
    emit(LoRa_AppPacketSent, loRaSF);
}

void SimpleLoRaApp::increaseSFIfPossible()
{
    if (loRaSF < 12)
        loRaSF++;
}

} //end namespace inet
