/*
 * ISLChannel.h
 *
 *  Created on: Jul 12, 2021
 *      Author: root
 */

#ifndef LORA_ISLCHANNEL_H_
#define LORA_ISLCHANNEL_H_

#include <omnetpp.h>
#include "inet/common/lifecycle/ILifecycle.h"
#include "inet/common/lifecycle/NodeStatus.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/lifecycle/LifecycleOperation.h"
#include "LoRaApp/LoRaAppPacket_m.h"
#include "LoRa/LoRaMacControlInfo_m.h"

namespace flora{

using namespace omnetpp;
using namespace inet;


class ISLChannel : public cSimpleModule
{
public:
    ISLChannel() {};
    void lookForDistance();
    int distance;
    //cMessage *timeee;
  //protected:
    // The following redefined virtual function holds the algorithm.
    virtual void initialize(int stage) override;
    virtual void setDistance(double distance);
    //virtual void handleSelfMessage(cMessage *msg);
    //virtual void handleMessage(cMessage *msg);
};
}



#endif /* LORA_ISLCHANNEL_H_ */
