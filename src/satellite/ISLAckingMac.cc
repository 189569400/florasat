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
    AckingMac::initialize(stage);
}

void ISLAckingMac::handleUpperPacket(Packet *packet)
{
    AckingMac::handleUpperPacket(packet);
}

void ISLAckingMac::handleLowerPacket(Packet *packet)
{
    AckingMac::handleLowerPacket(packet);
}

void ISLAckingMac::handleSelfMessage(cMessage *message)
{
    AckingMac::handleSelfMessage(message);
}

void ISLAckingMac::startTransmitting()
{
    // if there's any control info, remove it; then encapsulate the packet
    //MacAddress dest = currentTxFrame->getTag<MacAddressReq>()->getDestAddress();
    Packet *msg = currentTxFrame;
    MacAddress dest = msg->peekAtFront<LoRaMacFrame>()->getReceiverAddress();
    if (useAck && !dest.isBroadcast() && !dest.isMulticast() && !dest.isUnspecified()) { // unicast
        msg = currentTxFrame->dup();
        scheduleAfter(ackTimeout, ackTimeoutMsg);
    }
    else
        currentTxFrame = nullptr;

    encapsulate(msg);

    // send
    EV << "Starting transmission of " << msg << endl;
    radio->setRadioMode(fullDuplex ? IRadio::RADIO_MODE_TRANSCEIVER : IRadio::RADIO_MODE_TRANSMITTER);
    sendDown(msg);
}

void ISLAckingMac::encapsulate(Packet *packet)
{
    auto macHeader = makeShared<AckingMacHeader>();
    macHeader->setChunkLength(B(headerLength));

    MacAddress src = packet->peekAtFront<LoRaMacFrame>()->getTransmitterAddress();
    MacAddress dest = packet->peekAtFront<LoRaMacFrame>()->getReceiverAddress();

    macHeader->setSrc(src);
    macHeader->setDest(dest);

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
}

}
