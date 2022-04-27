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
            forwardToSatellite(pkt);
    }

    // message from GroundStation
    if (msg->arrivedOn("lowerLayerGS$i"))
    {
        EV << "Received GroundStation packet" << endl;
        SetupRoute(pkt, DOWNLINK);

        if (loraNodeAvailable())
            forwardToNode(pkt);
        else
            forwardToSatellite(pkt);
    }

    // message from another satellite
    if (msg->arrivedOn("lowerLayerISLIn"))
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
            //if (frame->getReceiverAddress() == MacAddress::BROADCAST_ADDRESS)
            //    processLoraMACPacket(pkt);
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
    //EV << frame->getTransmitterAddress() << endl;
    //for (std::vector<nodeEntry>::iterator it = knownNodes.begin() ; it != knownNodes.end(); ++it)

    // FIXME : Identify network server message is destined for.
    //L3Address destAddr = destAddresses[0];
    //if (pk->getControlInfo())
    //   delete pk->removeControlInfo();

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
    return satIndex == 15;
}

bool PacketHandler::loraNodeAvailable()
{
    return true;
}

void PacketHandler::forwardToGround(Packet *pkt)
{
    send(pkt, "lowerLayerGS$o");
}

void PacketHandler::forwardToNode(Packet *pkt)
{
    send(pkt, "lowerLayerLoRaOut");
}

void PacketHandler::forwardToSatellite(Packet *pkt)
{
    const auto &frame = pkt->peekAtFront<LoRaMacFrame>();
    int sourceSat = frame->getRoute(frame->getNumHop()-1);
    int macFrameType = frame->getPktType();

    cGate *islOut = gate("lowerLayerISLOut");

    // message already hopped in this satellite, forward packet
    if (sourceSat == satIndex)
        send(pkt, islOut);

    // if message comes from leftsat or downsat
    else if (sourceSat == satLeftIndex || sourceSat == satDownIndex)
    {
        // forward packet if it is uplink
        if (macFrameType == UPLINK)
            send(pkt, islOut);

        // if packet is downlink delete it
        else
            delete pkt;
    }

    // if message comes from rightsat or upsat
    else if (sourceSat == satRightIndex || sourceSat == satUpIndex)
    {
        // forward packet if it is downlink
        if (macFrameType == DOWNLINK)
            send(pkt, islOut);

        // if packet is uplink delete it
        else
            delete pkt;
    }

    // in other case the packet was sent by a further satellite
    else
        delete pkt;

}

}

