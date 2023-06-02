/*
 * Ipv4Interceptor.h
 *
 *  Created on: Mai 30, 2023
 *      Author: Robin Ohs
 */

#ifndef __FLORA_NETWORKLAYER_IPV4_INTERCEPTOR_H_
#define __FLORA_NETWORKLAYER_IPV4_INTERCEPTOR_H_

#include "inet/networklayer/ipv4/Ipv4.h"
#include "inet/networklayer/ipv4/Ipv4Header_m.h"
#include "networklayer/ConstellationRoutingTable.h"

using namespace inet;

namespace flora {
namespace networklayer {

/**
 * Intercepts IPv4.
 */
class Ipv4Interceptor : public cSimpleModule, public NetfilterBase::HookBase {
   public:
    Ipv4Interceptor();
    ~Ipv4Interceptor();

   protected:
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void finish() override;

    virtual INetfilter::IHook::Result datagramPreRoutingHook(Packet *datagram) override;
    virtual INetfilter::IHook::Result datagramForwardHook(Packet *datagram) override;
    virtual INetfilter::IHook::Result datagramPostRoutingHook(Packet *datagram) override;
    virtual INetfilter::IHook::Result datagramLocalInHook(Packet *datagram) override;
    virtual INetfilter::IHook::Result datagramLocalOutHook(Packet *datagram) override;

    virtual void handleMessage(cMessage *msg) override;

   protected:
    INetfilter *networkProtocol;
    ConstellationRoutingTable *routingTable;
    IIpv4RoutingTable *rt = nullptr;
    IInterfaceTable *ift = nullptr;
    int gsId = -1;
    L3Address routerAddr;
};

}  // namespace networklayer
}  // namespace flora

#endif
