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

#include "LoRaGWMac.h"
#include "inet/common/ModuleAccess.h"
#include "../LoRaPhy/LoRaPhyPreamble_m.h"
#include "inet/common/ProtocolTag_m.h"
#include "LoRaApp/SimpleLoRaApp.h"
#include "ISLChannel.h"
#include "PacketForwarder.h"
#include "inet/mobility/single/BonnMotionMobility.h"
#include <cmath>

#include <string.h>
//#include <omnetpp.h>
#include "ISLChannel.h"
#include <iostream>
#include <fstream>


#include <cstring>
#include <sstream>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>


#include "inet/physicallayer/wireless/common/contract/packetlevel/IRadio.h"

using namespace std;

namespace flora {

Define_Module(LoRaGWMac);

void LoRaGWMac::initialize(int stage)
{
    MacProtocolBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL)
    {
        outGate = findGate("upperMgmtOut");

        // subscribe for the information of the carrier sense
        cModule *radioModule = getModuleFromPar<cModule>(par("radioModule"), this);
        //radioModule->subscribe(IRadio::radioModeChangedSignal, this);
        radioModule->subscribe(IRadio::transmissionStateChangedSignal, this);
        radio = check_and_cast<IRadio *>(radioModule);

        waitingForDC = false;
        dutyCycleTimer = new cMessage("Duty Cycle Timer");
        beaconPeriod = new cMessage("Beacon Timer");
        beaconTimer = par("beaconTimer");
        pingNumber = par("pingNumber");
        //pingPeriod = par("PingPeriod");

        const char *usedClass = par("classUsed");
        if (!strcmp(usedClass,"B"))
            scheduleAt(simTime() + 1, beaconPeriod);

        //sendBeacon();
        const char *addressString = par("address");
        GW_forwardedDown = 0;
        GW_droppedDC = 0;
        if (!strcmp(addressString, "auto"))
        {
            // assign automatic address
            address = MacAddress::generateAutoAddress();
            // change module parameter from "auto" to concrete address
            par("address").setStringValue(address.str().c_str());
        }
        else
            address.setAddress(addressString);
    }
    else if (stage == INITSTAGE_LINK_LAYER) {
        radio->setRadioMode(IRadio::RADIO_MODE_TRANSCEIVER);
    }
}

void LoRaGWMac::finish()
{
    recordScalar("GW_forwardedDown", GW_forwardedDown);
    recordScalar("GW_droppedDC", GW_droppedDC);
    cancelAndDelete(dutyCycleTimer);
    cancelAndDelete(beaconPeriod);
    //cancelAndDelete(updateISLDistance);
}


void LoRaGWMac::configureNetworkInterface()
{

    MacAddress address = parseMacAddressParameter(par("address"));

    // generate a link-layer address to be used as interface token for IPv6
    networkInterface->setMacAddress(address);

    // capabilities
    networkInterface->setMtu(par("mtu"));
    networkInterface->setMulticast(true);
    networkInterface->setBroadcast(true);
    networkInterface->setPointToPoint(false);
}

void LoRaGWMac::handleSelfMessage(cMessage *msg)
{
    if(msg == beaconPeriod)
    {
        scheduleAt(simTime() + beaconTimer, beaconPeriod);
        sendBeacon();
    }
    if(msg == dutyCycleTimer)
        waitingForDC = false;
}

void LoRaGWMac::handleUpperMessage(cMessage *msg)
{
    if(waitingForDC == false)
    {
        auto pkt = check_and_cast<Packet *>(msg);
        const auto &frame = pkt->peekAtFront<LoRaMacFrame>();
        if (pkt->getControlInfo())
            delete pkt->removeControlInfo();

        auto tag = pkt->addTagIfAbsent<MacAddressReq>();

        tag->setDestAddress(frame->getReceiverAddress());


        waitingForDC = true;
        double delta;
        if(frame->getLoRaSF() == 7) delta = 0.61696;
        if(frame->getLoRaSF() == 8) delta = 1.23392;
        if(frame->getLoRaSF() == 9) delta = 2.14016;
        if(frame->getLoRaSF() == 10) delta = 4.28032;
        if(frame->getLoRaSF() == 11) delta = 7.24992;
        if(frame->getLoRaSF() == 12) delta = 14.49984;
        scheduleAt(simTime() + delta, dutyCycleTimer);
        GW_forwardedDown++;
        pkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::apskPhy);

        //if(!strcmp(usedClass,"B"))
        //{
          //  sendAtNextPingSlot(pkt);
            //scheduleAt(beaconTimer)
        //}
        //else{

        sendDown(pkt);

        //}
    }
    else
    {
        GW_droppedDC++;
        delete msg;
    }
}

void LoRaGWMac::handleLowerMessage(cMessage *msg)
{
    auto pkt = check_and_cast<Packet *>(msg);
    auto header = pkt->popAtFront<LoRaPhyPreamble>();
    const auto &frame = pkt->peekAtFront<LoRaMacFrame>();

    if(frame->getReceiverAddress() == MacAddress::BROADCAST_ADDRESS)
        sendUp(pkt);
    else
        delete pkt;
}

void LoRaGWMac::sendPacketBack(Packet *receivedFrame)
{
    const auto &frame = receivedFrame->peekAtFront<LoRaMacFrame>();
    EV << "sending Data frame back" << endl;
    auto pktBack = new Packet("LoraPacket");
    auto frameToSend = makeShared<LoRaMacFrame>();
    frameToSend->setChunkLength(B(par("headerLength").intValue()));

    frameToSend->setReceiverAddress(frame->getTransmitterAddress());
    pktBack->insertAtFront(frameToSend);
    sendDown(pktBack);
}

//this function send beacon message when the class is set to B
void LoRaGWMac::sendBeacon()
{
    auto beacon = new Packet("Beacon");
    auto frame = makeShared<LoRaMacFrame>();
    frame->setPktType(BEACON);

    //auto frame = makeShared<LoRaBeacon>();
    frame->setChunkLength(B(par("headerLength").intValue()));
    //const auto &frame = pkt->peekAtFront<LoRaMacFrame>();
    auto tag = beacon->addTagIfAbsent<MacAddressReq>();
    tag->setDestAddress(MacAddress::BROADCAST_ADDRESS);
    beacon->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::apskPhy);
    int loRaSF = 12;
    frame->setLoRaSF(loRaSF);
    units::values::Hz loRaBW = inet::units::values::Hz(125000);
    frame->setLoRaBW(loRaBW);
    units::values::Hz loRaCF = inet::units::values::Hz(868000000);
    frame->setLoRaCF(loRaCF);
    double loRaTP = 50;
    frame->setLoRaTP(loRaTP);
    frame->setBeaconTimer(beaconTimer);
    frame->setPingNb(pingNumber);

    beacon->insertAtFront(frame);
    sendDown(beacon);
}


void LoRaGWMac::receiveSignal(cComponent *source, simsignal_t signalID, intval_t value, cObject *details)
{
    Enter_Method_Silent();
    if (signalID == IRadio::transmissionStateChangedSignal)
    {
        IRadio::TransmissionState newRadioTransmissionState = (IRadio::TransmissionState)value;

        if (transmissionState == IRadio::TRANSMISSION_STATE_TRANSMITTING && newRadioTransmissionState == IRadio::TRANSMISSION_STATE_IDLE)
            radio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);

        transmissionState = newRadioTransmissionState;
    }
}

MacAddress LoRaGWMac::getAddress()
{
    return address;
}

}
