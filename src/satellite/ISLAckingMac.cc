/*
 * ISLAckingMac.cc
 *
 *  Created on: Apr 19, 2022
 *      Author: diego
 */

#include "ISLAckingMac.h"

#include <stdio.h>
#include <string.h>

#include "inet/common/INETUtils.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/ProtocolGroup.h"
#include "inet/common/ProtocolTag_m.h"
#include "inet/common/packet/Packet.h"
#include "inet/linklayer/acking/AckingMacHeader_m.h"
#include "inet/linklayer/common/InterfaceTag_m.h"
#include "inet/linklayer/common/MacAddressTag_m.h"
#include "inet/networklayer/contract/IInterfaceTable.h"

#include "inet/linklayer/acking/AckingMac.h"
#include "LoRaPhy/LoRaPhyPreamble_m.h"
#include "LoRa/LoRaMacFrame_m.h"


namespace flora {

Define_Module(ISLAckingMac);

ISLAckingMac::ISLAckingMac()
{
}

ISLAckingMac::~ISLAckingMac()
{
    cancelAndDelete(ackTimeoutMsg);
}

void ISLAckingMac::initialize(int stage)
{
    MacProtocolBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        bitrate = par("bitrate");
        headerLength = par("headerLength");
        promiscuous = par("promiscuous");
        fullDuplex = par("fullDuplex");
        useAck = par("useAck");
        ackTimeout = par("ackTimeout");

        cModule *radioModule = gate("lowerLayerOut")->getPathEndGate()->getOwnerModule();
        radioModule->subscribe(IRadio::transmissionStateChangedSignal, this);
        radio = check_and_cast<IRadio *>(radioModule);
        transmissionState = IRadio::TRANSMISSION_STATE_UNDEFINED;

        txQueue = check_and_cast<queueing::IPacketQueue *>(getSubmodule("queue"));
    }
    else if (stage == INITSTAGE_LINK_LAYER) {
        radio->setRadioMode(fullDuplex ? IRadio::RADIO_MODE_TRANSCEIVER : IRadio::RADIO_MODE_RECEIVER);
        if (useAck)
            ackTimeoutMsg = new cMessage("link-break");
        if (!txQueue->isEmpty()) {
            popTxQueue();
            startTransmitting();
        }
    }
}

void ISLAckingMac::encapsulate(Packet *packet)
{
    /*
    auto macHeader = makeShared<AckingMacHeader>();
    macHeader->setChunkLength(B(headerLength));
    auto macAddressReq = packet->getTag<MacAddressReq>();
    macHeader->setSrc(macAddressReq->getSrcAddress());
    macHeader->setDest(macAddressReq->getDestAddress());
    MacAddress dest = macAddressReq->getDestAddress();
    if (dest.isBroadcast() || dest.isMulticast() || dest.isUnspecified())
        macHeader->setSrcModuleId(-1);
    else
        macHeader->setSrcModuleId(getId());
    macHeader->setNetworkProtocol(ProtocolGroup::ethertype.getProtocolNumber(packet->getTag<PacketProtocolTag>()->getProtocol()));
    packet->insertAtFront(macHeader);
    auto macAddressInd = packet->addTagIfAbsent<MacAddressInd>();
    macAddressInd->setSrcAddress(macHeader->getSrc());
    macAddressInd->setDestAddress(macHeader->getDest());
    packet->getTagForUpdate<PacketProtocolTag>()->setProtocol(&Protocol::ackingMac);
    */
}

bool ISLAckingMac::dropFrameNotForUs(Packet *packet)
{
    /*
    auto macHeader = packet->peekAtFront<AckingMacHeader>();
    // Current implementation does not support the configuration of multicast
    // MAC address groups. We rather accept all multicast frames (just like they were
    // broadcasts) and pass it up to the higher layer where they will be dropped
    // if not needed.
    // All frames must be passed to the upper layer if the interface is
    // in promiscuous mode.

    if (macHeader->getDest().equals(networkInterface->getMacAddress()))
        return false;

    if (macHeader->getDest().isBroadcast())
        return false;

    if (promiscuous || macHeader->getDest().isMulticast())
        return false;

    EV << "Frame '" << packet->getName() << "' not destined to us, discarding\n";
    PacketDropDetails details;
    details.setReason(NOT_ADDRESSED_TO_US);
    emit(packetDroppedSignal, packet, &details);
    delete packet;
    */
    return true;
}

void ISLAckingMac::decapsulate(Packet *packet)
{

    const auto& macHeader = packet->popAtFront<AckingMacHeader>();
    auto macAddressInd = packet->addTagIfAbsent<MacAddressInd>();
    macAddressInd->setSrcAddress(macHeader->getSrc());
    macAddressInd->setDestAddress(macHeader->getDest());
    packet->addTagIfAbsent<InterfaceInd>()->setInterfaceId(networkInterface->getInterfaceId());
    auto payloadProtocol = ProtocolGroup::ethertype.getProtocol(macHeader->getNetworkProtocol());
    packet->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(payloadProtocol);
    packet->addTagIfAbsent<PacketProtocolTag>()->setProtocol(payloadProtocol);

}

void ISLAckingMac::handleUpperPacket(Packet *packet)
{
    const auto &frame = packet->peekAtFront<LoRaMacFrame>();
    auto tag = packet->addTagIfAbsent<MacAddressReq>();
    tag->setDestAddress(frame->getReceiverAddress());
    packet->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::apskPhy);

    EV << "Received " << packet << " for transmission\n";
    txQueue->enqueuePacket(packet);
    if (currentTxFrame || radio->getTransmissionState() == IRadio::TRANSMISSION_STATE_TRANSMITTING)
        EV << "Delaying transmission of " << packet << ".\n";
    else if (!txQueue->isEmpty()) {
        popTxQueue();
        startTransmitting();
    }
}

void ISLAckingMac::handleLowerPacket(Packet *packet)
{
    if (packet->hasBitError()) {
        EV << "Received frame '" << packet->getName() << "' contains bit errors or collision, dropping it\n";
        PacketDropDetails details;
        details.setReason(INCORRECTLY_RECEIVED);
        emit(packetDroppedSignal, packet, &details);
        delete packet;
        return;
    }

    // decapsulate and attach control info
    //decapsulate(packet);
    EV << "Passing up contained packet '" << packet->getName() << "' to higher layer\n";
    sendUp(packet);
}

void ISLAckingMac::handleSelfMessage(cMessage *message)
{
    if (message == ackTimeoutMsg) {
        EV_DETAIL << "AckingMac: timeout: " << currentTxFrame->getFullName() << " is lost\n";
        // packet lost
        emit(linkBrokenSignal, currentTxFrame);
        PacketDropDetails details;
        details.setReason(OTHER_PACKET_DROP);
        dropCurrentTxFrame(details);
        if (!txQueue->isEmpty()) {
            popTxQueue();
            startTransmitting();
        }
    }
    else {
        MacProtocolBase::handleSelfMessage(message);
    }
}


}
