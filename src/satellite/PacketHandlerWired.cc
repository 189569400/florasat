/*
 * PacketHandlerWired.cc
 *
 *  Created on: Jun 30, 2022
 *      Author: diego
 */

#include "PacketHandlerWired.h"
#include <cmath>
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/common/ModuleAccess.h"
#include "inet/applications/base/ApplicationPacket_m.h"
#include "inet/physicallayer/wireless/common/contract/packetlevel/SignalTag_m.h"

#include "inet/common/packet/printer/PacketPrinter.h"

#include "inet/linklayer/common/MacAddressTag_m.h"

#include "mobility/NoradA.h"
#include "mobility/INorad.h"

#include "../LoRaPhy/LoRaRadioControlInfo_m.h"
#include "../LoRaPhy/LoRaPhyPreamble_m.h"


namespace flora{

Define_Module(PacketHandlerWired);

void PacketHandlerWired::initialize(int stage)
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
            planes = noradAModule->getNumberOfPlanes();
            int satPerPlane = noradAModule->getSatellitesPerPlane();
            numOfSatellites = planes * satPerPlane;
            maxHops = planes + satPerPlane - 1;
            satPlane = trunc(satIndex/satPerPlane);
            int minSat = satPerPlane * satPlane;
            int maxSat = minSat + satPerPlane - 1;

            // satellite index of right ISL
            if (satPlane < planes-1)
                satRightIndex = satIndex + satPerPlane;

            else if (globalGrid && satPlane == planes-1)
                satRightIndex = satIndex % satPerPlane;

            // satellite index of left ISL
            if (0 < satPlane)
                satLeftIndex = satIndex - satPerPlane;

            else if (globalGrid && satPlane == 0)
                satLeftIndex = satIndex + (planes-1)*satPerPlane;

            // satellite index of up ISL
            if (satIndex+1 <= maxSat)
                satUpIndex = satIndex+1;

            else if (globalGrid && satIndex == maxSat)
                satUpIndex = minSat;

            // satellite index of down ISL
            if (minSat <= satIndex-1)
                satDownIndex = satIndex-1;

            else if (globalGrid && satIndex == minSat)
                satDownIndex = maxSat;
        }
        else
            throw cRuntimeError("NoradTLE mobility is not implemented yet");


        getSimulation()->getSystemModule()->subscribe("LoRa_AppPacketSent", this);
    }
}

void PacketHandlerWired::handleMessage(cMessage *msg)
{
    EV << msg->getArrivalGate() << endl;
    auto pkt = check_and_cast<Packet*>(msg);

    // message from LoRa Node
    if (msg->arrivedOn("lowerLayerLoRaIn"))
    {
        EV << "Received LoRaMAC frame" << endl;
        SetupRoute(pkt, UPLINK);
        processLoraMACPacket(pkt);

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
    if (msg->arrivedOn("up$i") || msg->arrivedOn("left$i") ||
            msg->arrivedOn("down$i") || msg->arrivedOn("right$i"))
    {
        auto frame = pkt->peekAtFront<LoRaMacFrame>();
        int macFrameType = frame->getPktType();

        if (macFrameType == UPLINK)
        {
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

void PacketHandlerWired::processLoraMACPacket(Packet *pk)
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
}

void PacketHandlerWired::sendPacket()
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


void PacketHandlerWired::receiveSignal(cComponent *source, simsignal_t signalID, intval_t value, cObject *details)
{
    if (simTime() >= getSimulation()->getWarmupPeriod())
        counterOfSentPacketsFromNodes++;
}

void PacketHandlerWired::finish()
{
    recordScalar("LoRa_GW_DER", double(counterOfReceivedPackets)/counterOfSentPacketsFromNodes);
    recordScalar("SatToGroundPkts", sentToGround);
    recordScalar("rcvdFromLoRa", counterOfReceivedPackets);
    recordScalar("rcvdFromLeftSat", rcvdFromLeftSat);
    recordScalar("rcvdFromDownSat", rcvdFromDownSat);
    recordScalar("rcvdFromRightSat", rcvdFromRightSat);
    recordScalar("rcvdFromUpSat", rcvdFromUpSat);
}


void PacketHandlerWired::SetupRoute(Packet *pkt, int macFrameType)
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


bool PacketHandlerWired::groundStationAvailable()
{
    // only last satellite in the constellation has ground station connection
    return satIndex == numOfSatellites-1;
}

bool PacketHandlerWired::loraNodeAvailable()
{
    return true;
}

void PacketHandlerWired::insertSatinRoute(Packet *pkt)
{
    auto frame = pkt->removeAtFront<LoRaMacFrame>();
    int numHops = frame->getNumHop();
    frame->setNumHop(numHops + 1);
    frame->setRoute(numHops, satIndex);
    frame->setTimestamps(numHops, simTime());
    pkt->insertAtFront(frame);
}

void PacketHandlerWired::forwardToGround(Packet *pkt)
{

    //PacketPrinter printer;
    //printer.printPacket(std::cout, pkt);

    auto frame = pkt->removeAtFront<LoRaMacFrame>();
    int sourceSat = frame->getRoute(frame->getNumHop()-1);
    int numHops = frame->getNumHop();

    // if message comes from leftsat or downsat or lora forward to ground
    if (sourceSat == satLeftIndex || sourceSat == satDownIndex)
    {
        frame->setNumHop(numHops + 1);
        frame->setRoute(numHops, satIndex);
        frame->setTimestamps(numHops, simTime());
        pkt->insertAtFront(frame);
        send(pkt, "lowerLayerGS$o");
        sentToGround++;

        EV << "Forwarding packet to ground station from satellite " << satIndex << ". Previous satellite hops:" << endl;
        for(int h=0; h<numHops; h++)
            EV << "In satellite " << frame->getRoute(h) << " at time " << frame->getTimestamps(h) << endl;

    }
    // or if it comes from lora forward to ground
    else if (pkt->arrivedOn("lowerLayerLoRaIn"))
    {
        pkt->insertAtFront(frame);
        send(pkt, "lowerLayerGS$o");
        sentToGround++;

        EV << "Forwarding packet to ground station from satellite " << satIndex << endl;
        EV << "No previous satellite hops, Packet reached local satellite at time " << frame->getTimestamps(0) << endl;
    }

    // in other case the packet was sent by a further satellite
    else
        delete pkt;
}

void PacketHandlerWired::forwardToNode(Packet *pkt)
{
    insertSatinRoute(pkt);
    send(pkt, "lowerLayerLoRaOut");
}

void PacketHandlerWired::forwardToSatellite(Packet *pkt)
{
    auto frame = pkt->removeAtFront<LoRaMacFrame>();
    int sourceSat = frame->getRoute(frame->getNumHop()-1);
    int macFrameType = frame->getPktType();
    int numHops = frame->getNumHop();

    frame->setNumHop(numHops + 1);
    frame->setRoute(numHops, satIndex);
    frame->setTimestamps(numHops, simTime());
    pkt->insertAtFront(frame);


    // forward all uplink packets to the right then up
    if (macFrameType == UPLINK)
    {
        if (satPlane == planes-1)
            send(pkt->dup(), "up$o");
        else
            send(pkt->dup(), "right$o");
    }

    else if (macFrameType == DOWNLINK)
    {
        if (satPlane == 0)
            send(pkt->dup(), "down$o");
        else
            send(pkt->dup(), "left$o");
    }

    if (sourceSat == satLeftIndex)
        rcvdFromLeftSat++;
    else if (sourceSat == satDownIndex)
        rcvdFromDownSat++;
    else if (sourceSat == satRightIndex)
        rcvdFromRightSat++;
    else if (sourceSat == satUpIndex)
        rcvdFromUpSat++;
}

}
