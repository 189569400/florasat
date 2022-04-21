/*
 * PacketHandler.cc
 *
 *  Created on: Mar 30, 2022
 *      Author: diego
 */


#include "PacketHandler.h"
#include <cmath>
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/common/ModuleAccess.h"
#include "inet/applications/base/ApplicationPacket_m.h"
#include "inet/physicallayer/wireless/common/contract/packetlevel/SignalTag_m.h"

#include "mobility/NoradA.h"
#include "mobility/INorad.h"

#include "../LoRaPhy/LoRaRadioControlInfo_m.h"
#include "../LoRaPhy/LoRaPhyPreamble_m.h"

namespace flora {

Define_Module(PacketHandler);

void PacketHandler::initialize(int stage)
{
    if (stage == 0)
    {
        localPort = par("localPort");
        destPort = par("destPort");
        globalGrid = par("globalGrid");
        numOfGroundStations = getSystemModule()->par("nrOfGS");
        LoRa_GWPacketReceived = registerSignal("LoRa_GWPacketReceived");
    }

    else if (stage == INITSTAGE_APPLICATION_LAYER)
    {
        noradModule = check_and_cast<INorad*>(getParentModule()->getSubmodule("NoradModule"));
        if (NoradA* noradAModule = dynamic_cast<NoradA*>(noradModule))
        {
            satIndex = noradAModule->getSatelliteNumber();
            int planes = noradAModule->getNumberOfPlanes();
            int satPerPlane = noradAModule->getSatellitesPerPlane();
            maxHops = planes + satPerPlane - 2;
            int satPlane = trunc(satIndex/satPerPlane);
            int minSat = satPerPlane * satPlane;
            int maxSat = minSat + satPerPlane - 1;

            // satellite index of right ISL
            if (satPlane+1 <= planes)
                satRightIndex = satIndex + satPerPlane;

            else if (globalGrid && satPlane==planes)
                satRightIndex = satIndex % satPerPlane;

            // satellite index of left ISL
            if (0 <= satPlane-1)
                satLeftIndex = satIndex + satPerPlane;

            else if (globalGrid && satPlane==0)
                satLeftIndex = satIndex % satPerPlane;

            // satellite index of up ISL
            if (satIndex+1 <= maxSat)
                satUpIndex = satIndex+1;

            else if (globalGrid && satIndex==maxSat)
                satUpIndex = minSat;

            // satellite index of down ISL
            if (minSat <= satIndex-1)
                satDownIndex = satIndex-1;

            else if (globalGrid && satIndex==minSat)
                satDownIndex = maxSat;
        }


        getSimulation()->getSystemModule()->subscribe("LoRa_AppPacketSent", this);
    }
}

void PacketHandler::handleMessage(cMessage *msg)
{
    EV << msg->getArrivalGate() << endl;
    auto pkt = check_and_cast<Packet*>(msg);

    // message from LoRa Node
    if (msg->arrivedOn("lowerLayerLoRaIn"))
    {
        EV << "Received LoRaMAC frame" << endl;
        SetupRoute(pkt, UPLINK);

        if (groundStationAvailable())
            forwardToGround(pkt);
        else
        {
            forwardToSatellite(pkt);
        }
    }

    // message from GroundStation
    if (msg->arrivedOn("lowerLayerGSIn"))
    {
        EV << "Received GroundStation packet" << endl;
        SetupRoute(pkt, DOWNLINK);

        if (loraNodeAvailable())
            forwardToNode(pkt);
        else
        {
            forwardToSatellite(pkt);
        }
    }

    // message from another satellite
    else
    {
        pkt->trimFront();
        auto frame = pkt->removeAtFront<LoRaMacFrame>();
        int macFrameType = frame->getPktType();
        int numHops = frame->getNumHop();

        frame->setNumHop(numHops + 1);
        frame->setRoute(numHops, satIndex);
        frame->setTimestamps(numHops, simTime());

        pkt->insertAtFront(frame);

        if (macFrameType == UPLINK)
        {
            // why only if is UPLINK?
            if (frame->getReceiverAddress() == MacAddress::BROADCAST_ADDRESS)
                processLoraMACPacket(pkt);
            // this part above

            if (groundStationAvailable())
                forwardToGround(pkt);
            else
                forwardToSatellite(pkt);
        }

        else if (macFrameType == DOWNLINK)
        {
            if (frame == nullptr)
                throw cRuntimeError("Packet type error");

            if (loraNodeAvailable())
                forwardToNode(pkt);
            else
                forwardToSatellite(pkt);
        }
    }
}

void PacketHandler::processLoraMACPacket(Packet *pk)
{
    // FIXME: Change based on new implementation of MAC frame.
    emit(LoRa_GWPacketReceived, 42);
    if (simTime() >= getSimulation()->getWarmupPeriod())
        counterOfReceivedPackets++;

    pk->trimFront();
    auto frame = pk->removeAtFront<LoRaMacFrame>();
    auto snirInd = pk->getTag<SnirInd>();
    auto signalPowerInd = pk->getTag<SignalPowerInd>();
    W w_rssi = signalPowerInd->getPower();
    double rssi = w_rssi.get()*1000;
    frame->setRSSI(math::mW2dBmW(rssi));
    frame->setSNIR(snirInd->getMinimumSnir());
    pk->insertAtFront(frame);

    //bool exist = false;
    EV << frame->getTransmitterAddress() << endl;
    //for (std::vector<nodeEntry>::iterator it = knownNodes.begin() ; it != knownNodes.end(); ++it)

    // FIXME : Identify network server message is destined for.
    L3Address destAddr = destAddresses[0];
    if (pk->getControlInfo())
       delete pk->removeControlInfo();

    //FIX THIS SOCKETS ARE NOT SUPPORTED
    //socket.sendTo(pk, destAddr, destPort);
}

void PacketHandler::sendPacket()
{
//    LoRaAppPacket *mgmtCommand = new LoRaAppPacket("mgmtCommand");
//    mgmtCommand->setMsgType(TXCONFIG);
//    LoRaOptions newOptions;
//    newOptions.setLoRaTP(uniform(0.1, 1));
//    mgmtCommand->setOptions(newOptions);
//
//    LoRaMacFrame *response = new LoRaMacFrame("mgmtCommand");
//    response->encapsulate(mgmtCommand);
//    response->setLoRaTP(pk->getLoRaTP());
//    response->setLoRaCF(pk->getLoRaCF());
//    response->setLoRaSF(pk->getLoRaSF());
//    response->setLoRaBW(pk->getLoRaBW());
//    response->setReceiverAddress(pk->getTransmitterAddress());
//    send(response, "lowerLayerOut");

}


void PacketHandler::receiveSignal(cComponent *source, simsignal_t signalID, intval_t value, cObject *details)
{
    if (simTime() >= getSimulation()->getWarmupPeriod())
        counterOfSentPacketsFromNodes++;
}

void PacketHandler::finish()
{
    recordScalar("LoRa_GW_DER", double(counterOfReceivedPackets)/counterOfSentPacketsFromNodes);
}


void PacketHandler::SetupRoute(Packet *pkt, int macFrameType)
{
    pkt->trimFront();
    auto frame = pkt->removeAtFront<LoRaMacFrame>();
    int numHops = frame->getNumHop();  // 0 at this stage

    frame->setPktType(macFrameType);
    frame->setNumHop(numHops + 1);
    frame->setRouteArraySize(maxHops);
    frame->setTimestampsArraySize(maxHops);
    frame->setRoute(numHops, satIndex);
    frame->setTimestamps(numHops, simTime());

    pkt->insertAtFront(frame);
}


bool PacketHandler::groundStationAvailable()
{
    // only sat 15 has ground station connection
    if (satIndex == 15)
        return true;

    return false;
}

bool PacketHandler::loraNodeAvailable()
{
    return true;
}

void PacketHandler::forwardToGround(Packet *pkt)
{
    send(pkt, "lowerLayerGSOut");
}

void PacketHandler::forwardToNode(Packet *pkt)
{
    send(pkt, "lowerLayerLoRaOut");
}

void PacketHandler::forwardToSatellite(Packet *pkt)
{
    const auto &frame = pkt->peekAtFront<LoRaMacFrame>();
    int macFrameType = frame->getPktType();

    // forward to up or right gates
    if (macFrameType == UPLINK)
    {
        //cGate *satRightGate = getParentModule()->gate("right$o");
        //cGate *satUpGate = getParentModule()->gate("up$o");

        cGate *islRightGate = gate("islRight$o");
        cGate *islUpGate = gate("islUp$o");

        if (satRightIndex>=0)
        {
            cDatarateChannel *rightChannel = check_and_cast<cDatarateChannel*>
                                             (islRightGate->getTransmissionChannel());
            if (rightChannel->isBusy())
                scheduleAt(rightChannel->getTransmissionFinishTime(), pkt);
            else
                send(pkt, islRightGate);
        }

        else if (satUpIndex>=0)
        {
            cDatarateChannel *upChannel = check_and_cast<cDatarateChannel*>
                                          (islUpGate->getTransmissionChannel());
            if (upChannel->isBusy())
                scheduleAt(upChannel->getTransmissionFinishTime(), pkt);
            else
                send(pkt, islUpGate);
        }

        else
            EV_ERROR << "Satellite 15 cannot reach ground station" << endl;

    }


    // forward to down or left gates
    else if (macFrameType == DOWNLINK)
    {
        //cGate *satLeftGate = getParentModule()->gate("left$o");
        //cGate *satDownGate = getParentModule()->gate("down$o");

        cGate *islLeftGate = gate("islLeft$o");
        cGate *islDownGate = gate("islDown$o");

        if (satLeftIndex>=0)
        {
            cDatarateChannel *leftChannel = check_and_cast<cDatarateChannel*>
                                            (islLeftGate->getTransmissionChannel());
            if (leftChannel->isBusy())
                scheduleAt(leftChannel->getTransmissionFinishTime(), pkt);
            else
                send(pkt, islLeftGate);
        }

        else if (satDownIndex>=0)
        {
            cDatarateChannel *downChannel = check_and_cast<cDatarateChannel*>
                                            (islDownGate->getTransmissionChannel());
            if (downChannel->isBusy())
                scheduleAt(downChannel->getTransmissionFinishTime(), pkt);
            else
                send(pkt, islDownGate);
        }

        else
            EV_ERROR << "Satellite 0 cannot reach lora node" << endl;
    }

}

}

