#include <string.h>
#include <omnetpp.h>
#include "SatPart.h"

namespace flora{

// The module class needs to be registered with OMNeT++
Define_Module(SatPart);

void SatPart::initialize(){}

void SatPart::handleMessage(cMessage *msg)
{
    EV<<"I RECEIVED A MESSAAAAAAAAGE !!!"<<endl;
    //send(msg, "out");
}
}
