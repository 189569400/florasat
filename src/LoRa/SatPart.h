/*
 * ISLChannel.h
 *
 *  Created on: Jul 12, 2021
 *      Author: root
 */

#ifndef LORA_SATPART_H_
#define LORA_SATPART_H_


namespace flora{

using namespace omnetpp;


class SatPart : public cSimpleModule
{
public:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    int distance;
  //protected:
    // The following redefined virtual function holds the algorithm.
    //virtual void handleMessage(cMessage *msg);
};
}



#endif /* LORA_ISLCHANNEL_H_ */
