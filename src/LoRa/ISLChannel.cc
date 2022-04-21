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


//using namespace omnetpp;


using namespace std;

namespace flora {

Define_Module(ISLChannel);


void ISLChannel::initialize(int stage)
{
    cSimpleModule::initialize(stage);
    if (stage == INITSTAGE_LOCAL){
        distance = par("distance");
    }
    //timeee = new cMessage("teeeest");
    //scheduleAt(simTime() + 2, timeee);
}

//void ISLChannel::handleSelfMessage(cMessage *msg)
//{

//}

void ISLChannel::lookForDistance()
{

}

void ISLChannel::setDistance(double distance)
{
    this->distance = distance;
}

}

