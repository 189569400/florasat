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

/**
 * Derive the Txc1 class from cSimpleModule. In the Tictoc1 network,
 * both the `tic' and `toc' modules are Txc1 objects, created by OMNeT++
 * at the beginning of the simulation.
 */
using namespace std;

namespace flora {
// The module class needs to be registered with OMNeT++
Define_Module(ISLChannel);


void ISLChannel::initialize(int stage)
{
    cSimpleModule::initialize(stage);
    if (stage == INITSTAGE_LOCAL){
        distance = par("distance");
        //printf("HELOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO !!!!!!");
        //lookForDistance();
    }
    //timeee = new cMessage("teeeest");
    //scheduleAt(simTime() + 2, timeee);
}

//void ISLChannel::handleSelfMessage(cMessage *msg)
//{
    //distance = 200;
    //EV<<"I CHANGED THE DISTANCE !!!!!!!!!!"<<endl;
//}

void ISLChannel::lookForDistance()
{
    //EV<<"ELEPHANT"<<endl;
    //ifstream ip("/home/mehdy/LEO_0_0-LLA-Pos.csv");

    //if(!ip.is_open()) std::cout <<"ERROR: File Open" << '\n';

    //string TIME;
    //string LAT;
    //string LON;
    //string ALT;

    //while(ip.good()){
        //getline(ip,TIME,',');
        //getline(ip,LAT,',');
        //getline(ip,LON,',');
        //getline(ip,ALT,',');
        //std::cout<<"Lattitude: "<<LAT<< '\n';
    //}
    //ip.close();
}
void ISLChannel::setDistance(double distance)
{
    EV<<"AM I WORKING ??????"<<endl;
    this->distance = distance;
}
}

