#include <string.h>
#include <omnetpp.h>
#include "SatPart.h"

namespace flora{

// The module class needs to be registered with OMNeT++
Define_Module(SatPart);

void SatPart::initialize()
{

}

void SatPart::handleMessage(cMessage *msg)
{
    // The handleMessage() method is called whenever a message arrives
    // at the module. Here, we just send it to the other module, through
    // gate `out'. Because both `tic' and `toc' does the same, the message
    // will bounce between the two.
    EV<<"I RECEIVED A MESSAAAAAAAAGE !!!"<<endl;
    //send(msg, "out"); // send out the message
}
}
