//
// Copyright (C) 2016 OpenSim Ltd.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

#include <iostream>
#include <cstring>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <iostream>
#include <openssl/evp.h>
#include <openssl/conf.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <cmath>
#include "inet/common/ModuleAccess.h"
#include "inet/linklayer/common/Ieee802Ctrl.h"
#include "inet/linklayer/common/UserPriority.h"
#include "inet/linklayer/common/MacAddressTag_m.h"
#include "inet/linklayer/csmaca/CsmaCaMac.h"
#include "inet/common/ProtocolTag_m.h"
#include "inet/linklayer/common/InterfaceTag_m.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/transportlayer/common/L4PortTag_m.h"
#include "inet/transportlayer/contract/udp/UdpControlInfo_m.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/common/ModuleAccess.h"
#include "inet/applications/base/ApplicationPacket_m.h"
#include "inet/networklayer/common/L3Tools.h"
#include "inet/networklayer/ipv4/Ipv4Header_m.h"

#include "LoRaMac.h"
#include "LoRaTagInfo_m.h"

using namespace std;

namespace flora {

Define_Module(LoRaMac);

LoRaMac::~LoRaMac()
{
    cancelAndDelete(endTransmission);
    cancelAndDelete(endReception);
    cancelAndDelete(droppedPacket);
    cancelAndDelete(pingPeriod);
    cancelAndDelete(beaconPeriod);
    cancelAndDelete(endBeaconReception);
    cancelAndDelete(beaconGuardStart);
    cancelAndDelete(beaconGuardEnd);
    cancelAndDelete(endPingSlot);
    cancelAndDelete(endDelay_1);
    cancelAndDelete(endListening_1);
    cancelAndDelete(endDelay_2);
    cancelAndDelete(endListening_2);
    cancelAndDelete(mediumStateChange);
}

/****************************************************************
 * Initialization functions.
 */
void LoRaMac::initialize(int stage)
{
    MacProtocolBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        EV << "Initializing stage 0\n";

        //maxQueueSize = par("maxQueueSize");
        headerLength = par("headerLength");
        ackLength = par("ackLength");
        ackTimeout = par("ackTimeout");
        retryLimit = par("retryLimit");

        // RX1 RX2 receive windows parameters
        waitDelay1Time = par("waitDelay1Time");
        listening1Time = par("listening1Time");
        waitDelay2Time = par("waitDelay2Time");
        listening2Time = par("listening2Time");

        // beacon parameters
        beaconStart = par("beaconStart");
        beaconPeriodTime = par("beaconPeriodTime");
        beaconReservedTime = par("beaconReservedTime");
        beaconGuardTime = par("beaconGuardTime");

        // class B parameters
        classBslotTime = par("classBslotTime");
        timeToNextSlot = par("timeToNextSlot");
        pingOffset = par("pingOffset");

        // class S parameters
        maxToA = par("maxToA");
        clockThreshold = par("clockThreshold");
        classSslotTime = 2*clockThreshold + maxToA;
        // with current parameters, 67 slots
        maxClassSslots = floor((beaconPeriodTime - beaconGuardTime - beaconReservedTime) / classSslotTime);

        slotSelectionData.setName("ClassSTXSlotSelection");
        //slotBeginTimes.setName("slotBeginTimes");


        const char *addressString = par("address");
        if (!strcmp(addressString, "auto")) {
            // assign automatic address
            address = MacAddress::generateAutoAddress();
            // change module parameter from "auto" to concrete address
            par("address").setStringValue(address.str().c_str());
        }
        else
            address.setAddress(addressString);

        // subscribe for the information of the carrier sense
        cModule *radioModule = getModuleFromPar<cModule>(par("radioModule"), this);
        radioModule->subscribe(IRadio::receptionStateChangedSignal, this);
        radioModule->subscribe(IRadio::transmissionStateChangedSignal, this);
        radioModule->subscribe(LoRaRadio::droppedPacket, this);
        radio = check_and_cast<IRadio *>(radioModule);

        // initialize self messages
        mediumStateChange = new cMessage("MediumStateChange");
        endTransmission = new cMessage("Transmission");
        endReception = new cMessage("Reception");
        droppedPacket = new cMessage("Dropped Packet");

        endDelay_1 = new cMessage("Delay_1");
        endListening_1 = new cMessage("Listening_1");
        endDelay_2 = new cMessage("Delay_2");
        endListening_2 = new cMessage("Listening_2");

        beaconGuardStart = new cMessage("Beacon_Guard_Start");
        beaconGuardEnd = new cMessage("Beacon_Guard_End");
        beaconPeriod = new cMessage("Beacon_Period");
        endBeaconReception = new cMessage("Beacon_Close");

        pingPeriod = new cMessage("Ping_Period");
        endPingSlot = new cMessage("Ping_Slot_Close");

        beginTXslot = new cMessage("UplinkSlot_Start");


        // set up internal queue
        txQueue = check_and_cast<queueing::IPacketQueue *>(getSubmodule("queue"));

        // schedule beacon when using class B or S
        const char *usedClass = par("classUsed");
        if (strcmp(usedClass,"A"))
        {
            scheduleAt(simTime() + beaconStart, beaconPeriod);
            scheduleAt(simTime() + beaconStart + beaconReservedTime, endBeaconReception);
            isClassA = false;

            if (!strcmp(usedClass,"B"))
                isClassB = true;

            if (!strcmp(usedClass,"S"))
                isClassS = true;
        }

        // state variables
        fsm.setName("LoRaMac State Machine");
        backoffPeriod = -1;
        retryCounter = 0;

        // sequence number for messages
        sequenceNumber = 0;
        bdw = 0;

        // statistics
        numRetry = 0;
        numSentWithoutRetry = 0;
        numGivenUp = 0;
        numCollision = 0;
        numSent = 0;
        numReceived = 0;
        numSentBroadcast = 0;
        numReceivedBroadcast = 0;
        numReceivedBeacons = 0;
        DPAD = 0;

        // initialize watches
        WATCH(fsm);
        WATCH(backoffPeriod);
        WATCH(retryCounter);
        WATCH(numRetry);
        WATCH(DPAD);
        WATCH(numSentWithoutRetry);
        WATCH(numGivenUp);
        WATCH(numCollision);
        WATCH(numSent);
        WATCH(numReceived);
        WATCH(numSentBroadcast);
        WATCH(numReceivedBroadcast);
    }

    else if (stage == INITSTAGE_LINK_LAYER)
    {
        radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
        macDPAD = registerSignal("macDPAD");
        macDPADOwnTraffic = registerSignal("macDPADOwnTraffic");
    }

}

void LoRaMac::finish()
{
    recordScalar("numRetry", numRetry);
    recordScalar("DPAD", DPAD);
    recordScalar("numSentWithoutRetry", numSentWithoutRetry);
    recordScalar("numGivenUp", numGivenUp);
    //recordScalar("numCollision", numCollision);
    recordScalar("numSent", numSent);
    recordScalar("numReceived", numReceived);
    recordScalar("numSentBroadcast", numSentBroadcast);
    recordScalar("numReceivedBroadcast", numReceivedBroadcast);
    recordScalar("numReceivedBeacons", numReceivedBeacons);
}

void LoRaMac::configureNetworkInterface()
{
    //NetworkInterface *e = new NetworkInterface(this);

    // data rate
    networkInterface->setDatarate(bitrate);
    networkInterface->setMacAddress(address);

    // capabilities
    //interfaceEntry->setMtu(par("mtu"));
    networkInterface->setMtu(std::numeric_limits<int>::quiet_NaN());
    networkInterface->setMulticast(true);
    networkInterface->setBroadcast(true);
    networkInterface->setPointToPoint(false);
}
/****************************************************************
 * Message handling functions.
 */
void LoRaMac::handleSelfMessage(cMessage *msg)
{
    if (msg == endBeaconReception)
    {
        beaconScheduling();
        if(iGotBeacon)
        {
            numReceivedBeacons++;
            iGotBeacon = false;
            if (isClassB)
                schedulePingPeriod();
            if (isClassS)
                scheduleULslots();

            if (hasGUI())
                getParentModule()->getParentModule()->bubble(beaconReceivedText);
        }
    }

    if (msg == beaconGuardStart)
        beaconGuard = true;

    if (msg == beaconGuardEnd)
        beaconGuard = false;

    if (msg == beginTXslot)
    {
        //classSslotCounter++;
        //scheduleAt(simTime() + classSslotTime, beginTXslot);
        //slotBeginTimes.record(simTime());
    }

    if (msg == endPingSlot)
    {
        simtime_t nextTime = (pingOffset*classBslotTime) + timeToNextSlot - classBslotTime;
        EV << "scheduling Next Ping Slot at " << simTime() + nextTime << endl;
        scheduleAt(simTime() + nextTime, pingPeriod);
        scheduleAt(simTime() + nextTime + classBslotTime, endPingSlot);
    }

    handleWithFsm(msg);
}

void LoRaMac::handleUpperMessage(cMessage *msg)
{
    auto pkt = check_and_cast<Packet *>(msg);
    pkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::apskPhy);
    auto pktEncap = encapsulate(pkt);
    const auto &frame = pktEncap->peekAtFront<LoRaMacFrame>();
    txQueue->enqueuePacket(pktEncap);

    EV << "frame " << pktEncap << " received from higher layer, receiver = " << frame->getReceiverAddress() << endl;

    if (fsm.getState() != IDLE || isClassS)
        EV << "deferring upper message transmission in " << fsm.getStateName() << " state\n";
    else
    {
        popTxQueue();
        handleWithFsm(currentTxFrame);
    }
}

void LoRaMac::handleLowerMessage(cMessage *msg)
{
    if( (fsm.getState() == RECEIVING_1) || (fsm.getState() == RECEIVING_2) ||
            (fsm.getState()== RECEIVING) || (fsm.getState()==RECEIVING_BEACON) )
    {
        auto pkt = check_and_cast<Packet *>(msg);
        const auto &frame = pkt->peekAtFront<LoRaMacFrame>();

        if (isBeacon(frame))
        {
            int ping = pow(2,12)/frame->getPingNb();
            timeToNextSlot = ping*classBslotTime;
            EV << "time to next slot: " << timeToNextSlot<< endl;
        }

        if (isDownlink(frame))
        {
            EV << "RECEIVED A DOWNLINK MESSAGE WITH A DPAD OF : " << DPAD<<endl;
            emit(macDPAD, DPAD.dbl());
            if (isForUs(frame))
                emit(macDPADOwnTraffic, DPAD.dbl());
        }

        handleWithFsm(msg);
    }

    else
    {
        EV << "Received lower message but MAC FSM is not on a valid state for reception" << endl;
        EV << "Deleting message " << msg << endl;
        delete msg;
    }
}

void LoRaMac::handleWithFsm(cMessage *msg)
{
    Ptr<LoRaMacFrame>frame = nullptr;

    auto pkt = dynamic_cast<Packet *>(msg);
    if (pkt)
    {
        const auto &chunk = pkt->peekAtFront<Chunk>();
        frame = dynamicPtrCast<LoRaMacFrame>(constPtrCast<Chunk>(chunk));
    }

    if (isClassA)
    {
        FSMA_Switch(fsm)
        {
            FSMA_State(IDLE)
            {
                FSMA_Enter(turnOffReceiver());
                FSMA_Event_Transition(Idle-Transmit,
                                      isUpperMessage(msg),
                                      TRANSMIT,
                                      EV << "CLASS A: starting transmission" << endl;
                                      );
            }

            FSMA_State(TRANSMIT)
            {
                FSMA_Enter(sendDataFrame(getCurrentTransmission()));
                FSMA_Event_Transition(Transmit-Wait_Delay_1,
                                      msg == endTransmission,
                                      WAIT_DELAY_1,
                                      EV << "CLASS A: transmission concluded" << endl;
                                      finishCurrentTransmission();
                                      numSent++;
                                      );
            }

            FSMA_State(WAIT_DELAY_1)
            {
                FSMA_Enter(turnOffReceiver());
                FSMA_Event_Transition(Wait_Delay_1-Listening_1,
                                      msg == endDelay_1 || endDelay_1->isScheduled() == false,
                                      LISTENING_1,
                                      EV << "CLASS A: opening receive window 1" << endl;
                                      );
            }

            FSMA_State(LISTENING_1)
            {
                FSMA_Enter(turnOnReceiver());
                FSMA_Event_Transition(Listening_1-Wait_Delay_2,
                                      msg == endListening_1 || endListening_1->isScheduled() == false,
                                      WAIT_DELAY_2,
                                      DPAD = simTime() - bdw;
                                      EV << "CLASS A: didn't receive downlink on receive window 1" << endl;
                                      );
                FSMA_Event_Transition(Listening_1-Receiving1,
                                      msg == mediumStateChange && isReceiving(),
                                      RECEIVING_1,
                                      EV << "CLASS A: receiving a message on receive window 1, analyzing packet..." << endl;
                                      DPAD = simTime() - bdw;
                                      );
            }

            FSMA_State(RECEIVING_1)
            {
                FSMA_Event_Transition(Receive-Unicast-Not-For,
                                      isLowerMessage(msg) && !isForUs(frame),
                                      LISTENING_1,
                                      EV << "CLASS A: wrong address downlink received, back to listening on window 1" << endl;
                                      );
                FSMA_Event_Transition(Receive-Unicast,
                                      isLowerMessage(msg) && isForUs(frame),
                                      IDLE,
                                      EV << "CLASS A: received downlink successfully on window 1, back to IDLE" << endl;
                                      sendUp(decapsulate(pkt));
                                      numReceived++;
                                      cancelEvent(endListening_1);
                                      cancelEvent(endDelay_2);
                                      cancelEvent(endListening_2);
                                      );
                FSMA_Event_Transition(Receive-BelowSensitivity,
                                      msg == droppedPacket,
                                      LISTENING_1,
                                      EV << "CLASS A: low power downlink, back to listening on window 1" << endl;
                                      );
            }

            FSMA_State(WAIT_DELAY_2)
            {
                FSMA_Enter(turnOffReceiver());
                FSMA_Event_Transition(Wait_Delay_2-Listening_2,
                                      msg == endDelay_2 || endDelay_2->isScheduled() == false,
                                      LISTENING_2,
                                      EV << "CLASS A: opening receive window 2" << endl;
                                      );
            }

            FSMA_State(LISTENING_2)
            {
                FSMA_Enter(turnOnReceiver());
                FSMA_Event_Transition(Listening_2-idle,
                                      msg == endListening_2 || endListening_2->isScheduled() == false,
                                      IDLE,
                                      EV << "CLASS A: didn't receive downlink on receive window 2" << endl;
                                      //DPAD = simTime() - bdw;
                                      );
                FSMA_Event_Transition(Listening_2-Receiving2,
                                      msg == mediumStateChange && isReceiving(),
                                      RECEIVING_2,
                                      EV << "CLASS A: receiving a message on receive window 2, analyzing packet..." << endl;
                                      DPAD = simTime() - bdw;
                                      );
            }

            FSMA_State(RECEIVING_2)
            {
                FSMA_Event_Transition(Receive2-Unicast-Not-For,
                                      isLowerMessage(msg) && !isForUs(frame),
                                      LISTENING_2,
                                      EV << "CLASS A: wrong address downlink received, back to listening on window 2" << endl;
                                      );
                FSMA_Event_Transition(Receive2-Unicast,
                                      isLowerMessage(msg) && isForUs(frame),
                                      IDLE,
                                      EV << "CLASS A: received downlink successfully on window 2, back to IDLE" << endl;
                                      sendUp(pkt);
                                      numReceived++;
                                      cancelEvent(endListening_2);
                                      );
                FSMA_Event_Transition(Receive2-BelowSensitivity,
                                      msg == droppedPacket,
                                      LISTENING_2,
                                      EV << "CLASS A: low power downlink, back to listening on window 2" << endl;
                                      );
            }
        }
    }

    // THE FSM FOR CLASS B
    if (isClassB)
    {
        FSMA_Switch(fsm)
        {
            FSMA_State(IDLE)
            {
                FSMA_Enter(turnOffReceiver());
                FSMA_Event_Transition(Idle-BeaconReception,
                                      msg == beaconPeriod,
                                      BEACON_RECEPTION,
                                      EV << "CLASS B: Going to Beacon Reception" << endl;
                                      );
                FSMA_Event_Transition(Idle-Transmit,
                                      isUpperMessage(msg),
                                      TRANSMIT,
                                      EV << "CLASS B: starting transmission" << endl;
                                      );
                FSMA_Event_Transition(Idle-ListeningOnPingSlot,
                                      msg == pingPeriod && !beaconGuard,
                                      PING_SLOT,
                                      EV << "CLASS B: starting Ping Slot" << endl;
                                      );
            }

            FSMA_State(BEACON_RECEPTION)
            {
                FSMA_Enter(turnOnReceiver());
                FSMA_Event_Transition(BeaconReception-Idle,
                                      msg == endBeaconReception,
                                      IDLE,
                                      EV << "CLASS B: no beacon detected, increasing beacon time" << endl;
                                      increaseBeaconTime();
                                      );
                FSMA_Event_Transition(BeaconReception-ReceivingBeacon,
                                      msg == mediumStateChange && isReceiving(),
                                      RECEIVING_BEACON,
                                      EV << "CLASS B: Going to Receiving Beacon" << endl;
                                      );
            }

            FSMA_State(RECEIVING_BEACON)
            {
                FSMA_Event_Transition(ReceivingBeacon-Unicast-Not-For,
                                      isLowerMessage(msg) && isBeacon(frame),  //  && !isForUs(frame)
                                      IDLE,
                                      EV << "CLASS B: beacon received" << endl;
                                      calculatePingPeriod(frame);
                                      );
                FSMA_Event_Transition(ReceivingBeacon-BelowSensitivity,
                                      msg == droppedPacket,
                                      IDLE,
                                      EV << "CLASS B: beacon below sensitivity" << endl;
                                      increaseBeaconTime();
                                      );
            }

            FSMA_State(PING_SLOT)
            {
                FSMA_Enter(turnOnReceiver());
                FSMA_Event_Transition(ListeningOnPingSlot-Idle,
                                      msg == endPingSlot && !isReceiving(),
                                      IDLE,
                                      EV << "CLASS B: no downlink detected, back to IDLE" << endl;
                                      );
                FSMA_Event_Transition(ListeningOnPingSlot-ReceivingOnPingSlot,
                                      msg == mediumStateChange && isReceiving(),
                                      RECEIVING,
                                      EV << "CLASS B: going to receive downlink on ping slot" << endl;
                                      );
            }

            FSMA_State(RECEIVING)
            {
                FSMA_Event_Transition(ReceivingOnPingSlot-Unicast-Not-For,
                                      isLowerMessage(msg) && !isForUs(frame),
                                      IDLE,
                                      EV << "CLASS B: wrong address downlink on ping slot, back to IDLE" << endl;
                                      );
                FSMA_Event_Transition(ReceivingOnPingSlot-Unicast,
                                      isLowerMessage(msg) && isForUs(frame),
                                      IDLE,
                                      EV << "CLASS B: received downlink on ping slot, back to IDLE" << endl;
                                      sendUp(decapsulate(pkt));
                                      numReceived++;
                                      );
                FSMA_Event_Transition(ReceivingOnPingSlot-BelowSensitivity,
                                      msg == droppedPacket,
                                      IDLE,
                                      EV << "CLASS B: downlink below sensitivity, back to IDLE" << endl;
                                      );
            }

            FSMA_State(TRANSMIT)
            {
                FSMA_Enter(sendDataFrame(getCurrentTransmission()));
                FSMA_Event_Transition(Transmit-Wait_Delay_1,
                                      msg == endTransmission,
                                      WAIT_DELAY_1,
                                      EV << "CLASS B: transmission concluded" << endl;
                                      finishCurrentTransmission();
                                      numSent++;
                                      );
            }

            FSMA_State(WAIT_DELAY_1)
            {
                FSMA_Enter(turnOffReceiver());
                FSMA_Event_Transition(Wait_Delay_1-Listening_1,
                                      msg == endDelay_1 || endDelay_1->isScheduled() == false,
                                      LISTENING_1,
                                      EV << "CLASS B: opening receive window 1" << endl;
                                      );
            }

            FSMA_State(LISTENING_1)
            {
                FSMA_Enter(turnOnReceiver());
                FSMA_Event_Transition(Listening_1-Wait_Delay_2,
                                      msg == endListening_1 || endListening_1->isScheduled() == false,
                                      WAIT_DELAY_2,
                                      EV << "CLASS B: didn't receive downlink on receive window 1" << endl;
                                      );
                FSMA_Event_Transition(Listening_1-Receiving1,
                                      msg == mediumStateChange && isReceiving(),
                                      RECEIVING_1,
                                      EV << "CLASS B: receiving a message on receive window 1, analyzing packet..." << endl;
                                      );
            }

            FSMA_State(RECEIVING_1)
            {
                FSMA_Event_Transition(Receive-Unicast-Not-For,
                                      isLowerMessage(msg) && !isForUs(frame),
                                      LISTENING_1,
                                      EV << "CLASS B: wrong address downlink received, back to listening on window 1" << endl;
                                      );
                FSMA_Event_Transition(Receive-Unicast,
                                      isLowerMessage(msg) && isForUs(frame),
                                      IDLE,
                                      EV << "CLASS B: received downlink successfully on window 1, back to IDLE" << endl;
                                      sendUp(decapsulate(pkt));
                                      numReceived++;
                                      cancelEvent(endListening_1);
                                      cancelEvent(endDelay_2);
                                      cancelEvent(endListening_2);
                                      );
                FSMA_Event_Transition(Receive-BelowSensitivity,
                                      msg == droppedPacket,
                                      LISTENING_1,
                                      EV << "CLASS B: low power downlink, back to listening on window 2" << endl;
                                      );
            }

            FSMA_State(WAIT_DELAY_2)
            {
                FSMA_Enter(turnOffReceiver());
                FSMA_Event_Transition(Wait_Delay_2-Listening_2,
                                      msg == endDelay_2 || endDelay_2->isScheduled() == false,
                                      LISTENING_2,
                                      EV << "CLASS B: opening receive window 2" << endl;
                                      );
            }

            FSMA_State(LISTENING_2)
            {
                FSMA_Enter(turnOnReceiver());
                FSMA_Event_Transition(Listening_2-idle,
                                      msg == endListening_2 || endListening_2->isScheduled() == false,
                                      IDLE,
                                      EV << "CLASS B: didn't receive downlink on receive window 2" << endl;
                                      );
                FSMA_Event_Transition(Listening_2-Receiving2,
                                      msg == mediumStateChange && isReceiving(),
                                      RECEIVING_2,
                                      EV << "CLASS B: receiving a message on receive window 2, analyzing packet..." << endl;
                                      );
            }

            FSMA_State(RECEIVING_2)
            {
                FSMA_Event_Transition(Receive2-Unicast-Not-For,
                                      isLowerMessage(msg) && !isForUs(frame),
                                      LISTENING_2,
                                      EV << "CLASS B: wrong address downlink received, back to listening on window 2" << endl;
                );
                FSMA_Event_Transition(Receive2-Unicast,
                                      isLowerMessage(msg) && isForUs(frame),
                                      IDLE,
                                      EV << "CLASS B: received downlink successfully on window 2, back to IDLE" << endl;
                                      sendUp(pkt);
                                      numReceived++;
                                      cancelEvent(endListening_2);
                                      );
                FSMA_Event_Transition(Receive2-BelowSensitivity,
                                      msg == droppedPacket,
                                      LISTENING_2,
                                      EV << "CLASS B: low power downlink, back to listening on window 2" << endl;
                                      );
            }
        }
    }

    // THE FSM FOR CLASS S
    if (isClassS)
    {
        FSMA_Switch(fsm)
        {
            FSMA_State(IDLE)
            {
                FSMA_Enter(turnOffReceiver());
                FSMA_Event_Transition(Idle-BeaconReception,
                                      msg == beaconPeriod,
                                      BEACON_RECEPTION,
                                      EV << "CLASS S: Going to Beacon Reception" << endl;
                                      );
                FSMA_Event_Transition(Idle-UplinkSlot,
                                      msg == beginTXslot && timeToTrasmit(),
                                      TRANSMIT,
                                      EV << "CLASS S: entering uplink slot" << endl;
                                      );
            }

            FSMA_State(BEACON_RECEPTION)
            {
                FSMA_Enter(turnOnReceiver());
                FSMA_Event_Transition(BeaconReception-Idle,
                                      msg == endBeaconReception,
                                      IDLE,
                                      EV << "CLASS S: no beacon detected, increasing beacon time" << endl;
                                      increaseBeaconTime();
                                      );
                FSMA_Event_Transition(BeaconReception-ReceivingBeacon,
                                      msg == mediumStateChange && isReceiving(),
                                      RECEIVING_BEACON,
                                      EV << "CLASS S: Going to Receiving Beacon" << endl;
                                      );
            }

            FSMA_State(RECEIVING_BEACON)
            {
                FSMA_Event_Transition(ReceivingBeacon-Unicast-Not-For,
                                      isLowerMessage(msg) && isBeacon(frame), // && !isForUs(frame)
                                      IDLE,
                                      EV << "CLASS S: beacon received" << endl;
                                      iGotBeacon = true;
                                      );
                FSMA_Event_Transition(ReceivingBeacon-BelowSensitivity,
                                      msg == droppedPacket,
                                      IDLE,
                                      EV << "CLASS S: beacon below sensitivity" << endl;
                                      increaseBeaconTime();
                                      );
            }

            FSMA_State(TRANSMIT)
            {
                FSMA_Enter(sendDataFrame(getCurrentTransmission()));
                FSMA_Event_Transition(Transmit-IDLE,
                                      msg == endTransmission,
                                      IDLE,
                                      EV << "CLASS S: transmission concluded" << endl;
                                      finishCurrentTransmission();
                                      numSent++;
                                      );
            }
        }
    }


    if (fsm.getState() == IDLE)
    {
        if (isReceiving())
            handleWithFsm(mediumStateChange);

        // in class S packets wait until the next uplink slot
        else if (currentTxFrame != nullptr && !isClassS)
            handleWithFsm(currentTxFrame);

        else if (!txQueue->isEmpty() && !isClassS)
        {
            popTxQueue();
            handleWithFsm(currentTxFrame);
        }
    }

    if (endSifs)
    {
        if (isLowerMessage(msg) && pkt->getOwner() == this && (endSifs->getContextPointer() != pkt))
            delete pkt;
    }

    else
    {
        if (isLowerMessage(msg) && pkt->getOwner() == this)
            delete pkt;
    }

    getDisplayString().setTagArg("t", 0, fsm.getStateName());
}

void LoRaMac::receiveSignal(cComponent *source, simsignal_t signalID, intval_t value, cObject *details)
{
    Enter_Method_Silent();
    if (signalID == IRadio::receptionStateChangedSignal)
    {
        IRadio::ReceptionState newRadioReceptionState = (IRadio::ReceptionState)value;
        if (receptionState == IRadio::RECEPTION_STATE_RECEIVING)
            radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);

        receptionState = newRadioReceptionState;
        handleWithFsm(mediumStateChange);
    }
    else if (signalID == LoRaRadio::droppedPacket)
    {
        //radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
        handleWithFsm(droppedPacket);
    }
    else if (signalID == IRadio::transmissionStateChangedSignal)
    {
        IRadio::TransmissionState newRadioTransmissionState = (IRadio::TransmissionState)value;
        if (transmissionState == IRadio::TRANSMISSION_STATE_TRANSMITTING && newRadioTransmissionState == IRadio::TRANSMISSION_STATE_IDLE)
        {
            handleWithFsm(endTransmission);
            radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
        }
        transmissionState = newRadioTransmissionState;
    }
}

Packet *LoRaMac::encapsulate(Packet *msg)
{
    auto frame = makeShared<LoRaMacFrame>();
    frame->setChunkLength(B(headerLength));
    msg->setArrival(msg->getArrivalModuleId(), msg->getArrivalGateId());
    auto tag = msg->getTag<LoRaTag>();

    frame->setTransmitterAddress(address);
    frame->setLoRaTP(tag->getPower().get());
    frame->setLoRaCF(tag->getCenterFrequency());
    frame->setLoRaSF(tag->getSpreadFactor());
    frame->setLoRaBW(tag->getBandwidth());
    frame->setLoRaCR(tag->getCodeRendundance());
    frame->setOriginTime(simTime());
    frame->setSequenceNumber(sequenceNumber);
    frame->setReceiverAddress(MacAddress::BROADCAST_ADDRESS);

    ++sequenceNumber;
    frame->setLoRaUseHeader(tag->getUseHeader());

    msg->insertAtFront(frame);

    return msg;
}

Packet *LoRaMac::decapsulate(Packet *frame)
{
    auto loraHeader = frame->popAtFront<LoRaMacFrame>();
    frame->addTagIfAbsent<MacAddressInd>()->setSrcAddress(loraHeader->getTransmitterAddress());
    frame->addTagIfAbsent<MacAddressInd>()->setDestAddress(loraHeader->getReceiverAddress());
    frame->addTagIfAbsent<InterfaceInd>()->setInterfaceId(networkInterface->getInterfaceId());
    return frame;
}

/****************************************************************
 * Frame sender functions.
 */
void LoRaMac::sendDataFrame(Packet *frameToSend)
{
    EV << "sending Data frame\n";
    radio->setRadioMode(IRadio::RADIO_MODE_TRANSMITTER);

    auto frameCopy = frameToSend->dup();

    //LoRaMacControlInfo *ctrl = new LoRaMacControlInfo();
    //ctrl->setSrc(frameCopy->getTransmitterAddress());
    //ctrl->setDest(frameCopy->getReceiverAddress());
    //frameCopy->setControlInfo(ctrl);

    auto macHeader = frameCopy->peekAtFront<LoRaMacFrame>();
    auto macAddressInd = frameCopy->addTagIfAbsent<MacAddressInd>();
    macAddressInd->setSrcAddress(macHeader->getTransmitterAddress());
    macAddressInd->setDestAddress(macHeader->getReceiverAddress());

    //frameCopy->addTag<PacketProtocolTag>()->setProtocol(&Protocol::lora);

    sendDown(frameCopy);
}

void LoRaMac::sendAckFrame()
{
    auto frameToAck = static_cast<Packet *>(endSifs->getContextPointer());
    endSifs->setContextPointer(nullptr);
    auto macHeader = makeShared<CsmaCaMacAckHeader>();
    macHeader->setReceiverAddress(MacAddress(frameToAck->peekAtFront<LoRaMacFrame>()->getTransmitterAddress().getInt()));

    EV << "sending Ack frame\n";
    //auto macHeader = makeShared<CsmaCaMacAckHeader>();
    macHeader->setChunkLength(B(ackLength));
    auto frame = new Packet("CsmaAck");
    frame->insertAtFront(macHeader);
    //frame->addTag<PacketProtocolTag>()->setProtocol(&Protocol::lora);
    radio->setRadioMode(IRadio::RADIO_MODE_TRANSMITTER);

    auto macAddressInd = frame->addTagIfAbsent<MacAddressInd>();
    macAddressInd->setSrcAddress(macHeader->getTransmitterAddress());
    macAddressInd->setDestAddress(macHeader->getReceiverAddress());

    sendDown(frame);
}

/****************************************************************
 * Helper functions.
 */

// schedule beacon signals
void LoRaMac::beaconScheduling()
{
    scheduleAt(simTime() + beaconPeriodTime, endBeaconReception);
    scheduleAt(simTime() + beaconPeriodTime - beaconReservedTime - beaconGuardTime, beaconGuardStart);
    scheduleAt(simTime() + beaconPeriodTime - beaconReservedTime, beaconGuardEnd);
    scheduleAt(simTime() + beaconPeriodTime - beaconReservedTime, beaconPeriod);
}

void LoRaMac::increaseBeaconTime()
{
    beaconReservedTime = beaconReservedTime + 1;
}

void LoRaMac::schedulePingPeriod()
{
    cancelEvent(pingPeriod);
    cancelEvent(endPingSlot);
    scheduleAt(simTime() + (pingOffset*classBslotTime), pingPeriod);
    scheduleAt(simTime() + (pingOffset*classBslotTime) + classBslotTime, endPingSlot);
}

void LoRaMac::scheduleULslots()
{
    // when beacon is received begin scheduling of uplink slots
    // and randomly determine the slot to be used during this beacon period
    classSslotCounter = -1;
    targetClassSslot = cComponent::intuniform(0, maxClassSslots-1);

    slotSelectionData.record(targetClassSslot);

    cancelEvent(beginTXslot);
    scheduleAt(simTime() + clockThreshold + targetClassSslot*classSslotTime, beginTXslot);
}

//calculate the pingSlotPeriod using Aes128 encryption for randomization
void LoRaMac::calculatePingPeriod(const Ptr<const LoRaMacFrame> &frame)
{
    iGotBeacon = true;
    beaconReservedTime = 2.120;
    unsigned char cipher[7];

    cipher[0]=(unsigned char)(frame->getBeaconTimer()
            + getAddress().getAddressByte(0)
            + getAddress().getAddressByte(1)
            + getAddress().getAddressByte(2)
            + getAddress().getAddressByte(3)
            + getAddress().getAddressByte(4)
            + getAddress().getAddressByte(5)
            );
    cipher[1]=(unsigned char)(getAddress().getAddressByte(0));
    cipher[2]=(unsigned char)(getAddress().getAddressByte(1));
    cipher[3]=(unsigned char)(getAddress().getAddressByte(2));
    cipher[4]=(unsigned char)(getAddress().getAddressByte(3));
    cipher[5]=(unsigned char)(getAddress().getAddressByte(4));
    cipher[6]=(unsigned char)(getAddress().getAddressByte(5));

    int message_len = strlen((const char*)cipher);
    unsigned char cipher2[64];
    unsigned char* key = (unsigned char*)"00000000000000000000000000000000";

    int cipher_len = aesEncrypt(cipher,message_len,key,cipher2);
    int period = pow(2,12)/(frame->getPingNb());

    pingOffset = (cipher2[0]+(cipher2[1]*256))% period;
}

int LoRaMac::aesEncrypt(unsigned char *message, int message_len, unsigned char *key, unsigned char *cipher)
{
    int cipher_len = 0;
    int len = 0;

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();

    if(!ctx){
        perror("EVP_SIPHER_CTX_new() failed");
        exit(-1);
    }
    if (!EVP_EncryptInit_ex(ctx, EVP_aes_128_ecb(), NULL, key, NULL)){
        perror("EVP_EncryptInit_ex() failed");
        exit(-1);
    }
    if (!EVP_EncryptUpdate(ctx, cipher, &len, message, message_len)){
        perror("EVP_EncryptUpdate() failed");
        exit(-1);
    }
    cipher_len += len;
    if (!EVP_EncryptFinal_ex(ctx, cipher + len, &len)){
        perror("EVP_EnryptFinal_ex() failed");
        exit(-1);
    }
    cipher_len += len;
    EVP_CIPHER_CTX_free(ctx);
    return cipher_len;
}

void LoRaMac::finishCurrentTransmission()
{
    deleteCurrentTxFrame();
    if (isClassS)
        return;

    bdw = simTime()+ waitDelay1Time;
    scheduleAt(simTime() + waitDelay1Time, endDelay_1);
    scheduleAt(simTime() + waitDelay1Time + listening1Time, endListening_1);
    scheduleAt(simTime() + waitDelay1Time + listening1Time + waitDelay2Time, endDelay_2);
    scheduleAt(simTime() + waitDelay1Time + listening1Time + waitDelay2Time + listening2Time, endListening_2);
}

Packet *LoRaMac::getCurrentTransmission()
{
    ASSERT(currentTxFrame != nullptr);
    return currentTxFrame;
}
/*Packet *LoRaMac::getCurrentReception()
{
    ASSERT(currentRxFrame != nullptr);
    return currentRxFrame;
}*/

bool LoRaMac::isReceiving()
{
    return radio->getReceptionState() == IRadio::RECEPTION_STATE_RECEIVING;
}

bool LoRaMac::isAck(const Ptr<const LoRaMacFrame> &frame)
{
    return false;//dynamic_cast<LoRaMacFrame *>(frame);
}

bool LoRaMac::isBeacon(const Ptr<const LoRaMacFrame> &frame)
{
    return frame->getPktType() == BEACON;
}

bool LoRaMac::isDownlink(const Ptr<const LoRaMacFrame> &frame)
{
    return frame->getPktType() == DOWNLINK;
}

bool LoRaMac::isBroadcast(const Ptr<const LoRaMacFrame> &frame)
{
    return frame->getReceiverAddress().isBroadcast();
}

bool LoRaMac::isForUs(const Ptr<const LoRaMacFrame> &frame)
{
    return frame->getReceiverAddress() == address;
}

// possible implementation of an uplink transmission policy
bool LoRaMac::timeToTrasmit()
{
    // if not in beacon guard period and
    // if there is a queued message and
    // if it is the corresponding slot
    if (!beaconGuard && !txQueue->isEmpty()) //&& classSslotCounter == targetClassSslot)
    {
        popTxQueue();
        return true;
    }

    return false;
}

void LoRaMac::turnOnReceiver()
{
    LoRaRadio *loraRadio;
    loraRadio = check_and_cast<LoRaRadio *>(radio);
    loraRadio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);
}

void LoRaMac::turnOffReceiver()
{
    LoRaRadio *loraRadio;
    loraRadio = check_and_cast<LoRaRadio *>(radio);
    loraRadio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
}

MacAddress LoRaMac::getAddress()
{
    return address;
}

} // namespace inet
