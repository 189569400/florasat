/*
 * Ipv4Interceptor.cc
 *
 *  Created on: Mai 30, 2023
 *      Author: Robin Ohs
 */

#include "Ipv4Interceptor.h"

#include "inet/linklayer/common/InterfaceTag_m.h"
#include "inet/networklayer/common/NextHopAddressTag_m.h"

namespace flora {
namespace networklayer {

Define_Module(Ipv4Interceptor);

Ipv4Interceptor::Ipv4Interceptor() {
    networkProtocol = nullptr;
}

Ipv4Interceptor::~Ipv4Interceptor() {
}

void Ipv4Interceptor::initialize(int stage) {
    if (stage == INITSTAGE_LOCAL) {
        networkProtocol = getModuleFromPar<INetfilter>(par("networkProtocolModule"), this);
        routingTable = check_and_cast<ConstellationRoutingTable *>(getSystemModule()->getSubmodule("constellationRoutingTable"));
        gsId = getParentModule()->getIndex();
        routerAddr = L3Address(getParentModule()->par("routerAddr").stringValue());
    } else if (stage == INITSTAGE_NETWORK_LAYER) {
        networkProtocol->registerHook(0, this);
    }
}

void Ipv4Interceptor::finish() {
    if (isRegisteredHook(networkProtocol))
        networkProtocol->unregisterHook(this);
}

void Ipv4Interceptor::handleMessage(cMessage *msg) {
    if (msg->isSelfMessage()) {
        Packet *context = (Packet *)msg->getContextPointer();
        delete msg;
        networkProtocol->reinjectQueuedDatagram(context);
    } else
        throw cRuntimeError("This module does not handle incoming messages");
}

INetfilter::IHook::Result Ipv4Interceptor::datagramPreRoutingHook(Packet *datagram) {
    return INetfilter::IHook::ACCEPT;
}

INetfilter::IHook::Result Ipv4Interceptor::datagramForwardHook(Packet *datagram) {
    Enter_Method("datagramForwardHook");
    // auto frame = datagram->peekAtFront<Ipv4Header>();
    // int dstGsId = routingTable->getGroundstationFromAddress(frame->getDestAddress());

    // if (dstGsId == gsId) {
    //     datagram->removeTag<NextHopAddressReq>();
    //     datagram->addTagIfAbsent<NextHopAddressReq>()->setNextHopAddress(routerAddr);
    //     return INetfilter::IHook::ACCEPT;
    // } else {
    //     send(datagram->dup(), "pgOut");
    //     return INetfilter::IHook::DROP;
    // }
    return INetfilter::IHook::ACCEPT;
}

INetfilter::IHook::Result Ipv4Interceptor::datagramPostRoutingHook(Packet *datagram) {
    return INetfilter::IHook::ACCEPT;
}

INetfilter::IHook::Result Ipv4Interceptor::datagramLocalInHook(Packet *datagram) {
    return INetfilter::IHook::ACCEPT;
}

INetfilter::IHook::Result Ipv4Interceptor::datagramLocalOutHook(Packet *datagram) {
    Enter_Method("datagramLocalOutHook");

    send(datagram->dup(), "pgOut");
    return INetfilter::IHook::DROP;
}

}  // namespace networklayer
}  // namespace flora
