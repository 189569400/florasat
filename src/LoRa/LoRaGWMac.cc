//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#include "LoRaGWMac.h"
#include "inet/common/ModuleAccess.h"
#include "../LoRaPhy/LoRaPhyPreamble_m.h"
#include "inet/common/ProtocolTag_m.h"
#include "LoRaApp/SimpleLoRaApp.h"
#include "ISLChannel.h"
#include "PacketForwarder.h"
#include "inet/mobility/single/BonnMotionMobility.h"
#include <cmath>

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


#include "inet/physicallayer/wireless/common/contract/packetlevel/IRadio.h"

using namespace std;

namespace flora {

Define_Module(LoRaGWMac);

void LoRaGWMac::initialize(int stage)
{
    MacProtocolBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        outGate=findGate("upperMgmtOut");
        // subscribe for the information of the carrier sense
        cModule *radioModule = getModuleFromPar<cModule>(par("radioModule"), this);
        //radioModule->subscribe(IRadio::radioModeChangedSignal, this);
        radioModule->subscribe(IRadio::transmissionStateChangedSignal, this);
        radio = check_and_cast<IRadio *>(radioModule);
        waitingForDC = false;
        dutyCycleTimer = new cMessage("Duty Cycle Timer");
        beaconPeriod = new cMessage("Beacon Timer");
        //calculDistance = new cMessage("Calculate the distance");
        const char *usedClass = par("classUsed");
        //satelliteID = par("satelliteID");
        beaconTimer = par("beaconTimer");
        //updateISLDistanceInterval = par("updateISLDistanceInterval");
        pingNumber = par("pingNumber");
        //pingPeriod = par("PingPeriod");
        //scheduleAt(simTime()+updateISLDistanceInterval,calculDistance);
        if (!strcmp(usedClass,"B")){
            scheduleAt(simTime() + 1, beaconPeriod);
            //int temp = simTime()+1;
        }
        //sendBeacon();
        const char *addressString = par("address");
        GW_forwardedDown = 0;
        GW_droppedDC = 0;
        if (!strcmp(addressString, "auto")) {
            // assign automatic address
            address = MacAddress::generateAutoAddress();
            // change module parameter from "auto" to concrete address
            par("address").setStringValue(address.str().c_str());
        }
        else
            address.setAddress(addressString);
    }
    else if (stage == INITSTAGE_LINK_LAYER) {
        radio->setRadioMode(IRadio::RADIO_MODE_TRANSCEIVER);
    }
}

void LoRaGWMac::finish()
{
    recordScalar("GW_forwardedDown", GW_forwardedDown);
    recordScalar("GW_droppedDC", GW_droppedDC);
    cancelAndDelete(dutyCycleTimer);
    cancelAndDelete(beaconPeriod);
    //cancelAndDelete(calculDistance);
}


void LoRaGWMac::configureNetworkInterface()
{
    //NetworkInterface *e = new NetworkInterface(this);

    // data rate
    //e->setDatarate(bitrate);

    // generate a link-layer address to be used as interface token for IPv6
    //e->setMACAddress(address);
    //e->setInterfaceToken(address.formInterfaceIdentifier());

    // capabilities
    //e->setMtu(par("mtu"));
    //e->setMulticast(true);
    //e->setBroadcast(true);
    //e->setPointToPoint(false);
    MacAddress address = parseMacAddressParameter(par("address"));

    // generate a link-layer address to be used as interface token for IPv6
    networkInterface->setMacAddress(address);
    // data rate
    //interfaceEntry->setDatarate(bitrate);

    // capabilities
    networkInterface->setMtu(par("mtu"));
    networkInterface->setMulticast(true);
    networkInterface->setBroadcast(true);
    networkInterface->setPointToPoint(false);
}

void LoRaGWMac::handleSelfMessage(cMessage *msg)
{
    EV<<"HELLOOOOO Mehdy !!" << endl;
/*    if(msg == calculDistance){
        //auto test = getContainingNode(this);
        //auto test2 = check_and_cast_nullable<BonnMotionMobility *>(test->getSubmodule("mobility"));
        //EV<<"THE POSITION OF THE GW IS :"<<test2->getCurrentPosition()<<endl;
        EV<<"MY ID IS "<<satelliteID<<endl;
        int devID = satelliteID;
        distanceCalculation(devID);
        scheduleAt(simTime()+updateISLDistanceInterval,calculDistance);

    }*/
    if(msg == beaconPeriod){
        scheduleAt(simTime() + beaconTimer, beaconPeriod);
        //temp = simTime()+beaconTimer;
        sendBeacon();
    }
    if(msg == dutyCycleTimer) waitingForDC = false;

}

void LoRaGWMac::handleUpperMessage(cMessage *msg)
{
    EV<<"HANDLING UPPER MESSAGE";
    if(waitingForDC == false)
    {
//        LoRaMacFrame *frame = check_and_cast<LoRaMacFrame *>(msg);
//        frame->removeControlInfo();
        auto pkt = check_and_cast<Packet *>(msg);
        const auto &frame = pkt->peekAtFront<LoRaMacFrame>();
        if (pkt->getControlInfo())
            delete pkt->removeControlInfo();

        auto tag = pkt->addTagIfAbsent<MacAddressReq>();

        tag->setDestAddress(frame->getReceiverAddress());
//        LoRaMacControlInfo *ctrl = new LoRaMacControlInfo();
//        ctrl->setSrc(address);
//        ctrl->setDest(frame->getReceiverAddress());
//        frame->setControlInfo(ctrl);
//        sendDown(frame);


        waitingForDC = true;
        double delta;
        if(frame->getLoRaSF() == 7) delta = 0.61696;
        if(frame->getLoRaSF() == 8) delta = 1.23392;
        if(frame->getLoRaSF() == 9) delta = 2.14016;
        if(frame->getLoRaSF() == 10) delta = 4.28032;
        if(frame->getLoRaSF() == 11) delta = 7.24992;
        if(frame->getLoRaSF() == 12) delta = 14.49984;
        scheduleAt(simTime() + delta, dutyCycleTimer);
        GW_forwardedDown++;
        pkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::apskPhy);
        //if(!strcmp(usedClass,"B"))
        //{
          //  sendAtNextPingSlot(pkt);
            //scheduleAt(beaconTimer)
        //}
        //else{
        sendDown(pkt);
        //}
    }
    else
    {
        GW_droppedDC++;
        delete msg;
    }
}

void LoRaGWMac::handleLowerMessage(cMessage *msg)
{
    EV<<"HANDLING LOWER MESSAGE";
    auto pkt = check_and_cast<Packet *>(msg);
    auto header = pkt->popAtFront<LoRaPhyPreamble>();
    cGate *gate2= gate("right$o");
    cGate *otherGate = gate2->getType()==cGate::OUTPUT ? gate2->getNextGate():
            gate2->getPreviousGate();
    if (otherGate)
        EV<<"GATE IS COONNEEECTEEED TO : "<< otherGate->getFullPath() <<endl;
    else
        EV<<"GATE IS NOOOOOT CONNECTED"<< endl;
    const auto &frame = pkt->peekAtFront<LoRaMacFrame>();

    auto test = getContainingNode(this);
    auto test4 = getContainingNode(test);
    //auto test5 = check_and_cast_nullable<ISLChannel *>(test->getSubmodule("ISLChannel"));
    auto test2 = check_and_cast_nullable<BonnMotionMobility *>(test->getSubmodule("mobility"));
    //test5->par("test9")=50;
    //test5->distance=50;
    //test2->test3 = 40000;

    //test->par("test8")=50;

    EV<<"THE POSITION OF THE GW IS :"<<test2->getCurrentPosition()<<endl;
    //test2->par("distance")=30000;

    //EV<<"THE NEW VALUE IS"<<test->par("test8");

    if(frame->getReceiverAddress() == MacAddress::BROADCAST_ADDRESS){
       // if(satelliteID==15)
            //sendUp(pkt);
        //else
            //send(pkt,"right");
            sendUp(pkt);
    }
    else
        delete pkt;
}

void LoRaGWMac::sendPacketBack(Packet *receivedFrame)
{
    const auto &frame = receivedFrame->peekAtFront<LoRaMacFrame>();
    EV << "sending Data frame back" << endl;
    auto pktBack = new Packet("LoraPacket");
    auto frameToSend = makeShared<LoRaMacFrame>();
    frameToSend->setChunkLength(B(par("headerLength").intValue()));

    frameToSend->setReceiverAddress(frame->getTransmitterAddress());
    pktBack->insertAtFront(frameToSend);
    sendDown(pktBack);
}

/*void LoRaGWMac::sendAtNextPingSlot(Packet *pkt)
{
    int l = temp - simTime();
    if (l <= pingOffset)
    {
        scheduleAt()
    }
    int meh = pow(2,12)/pingNumber;
    scheduleAt();

}*/
//this function send beacon message when the class is set to B
void LoRaGWMac::sendBeacon()
{
    //auto pkt = check_and_cast<Packet *>(msg);
    //beacon = new cMessage("Ack Timeout");
    auto beacon = new Packet("Beacon");
    auto frame = makeShared<LoRaMacFrame>();
    frame->setPktType(BEACON);
    //auto frame = makeShared<LoRaBeacon>();
    frame->setChunkLength(B(par("headerLength").intValue()));
    //const auto &frame = pkt->peekAtFront<LoRaMacFrame>();
    auto tag = beacon->addTagIfAbsent<MacAddressReq>();
    tag->setDestAddress(MacAddress::BROADCAST_ADDRESS);
    beacon->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::apskPhy);
    int loRaSF = 12;
    frame->setLoRaSF(loRaSF);
    units::values::Hz loRaBW = inet::units::values::Hz(125000);
    frame->setLoRaBW(loRaBW);
    units::values::Hz loRaCF = inet::units::values::Hz(868000000);
    frame->setLoRaCF(loRaCF);
    double loRaTP = 50;
    frame->setLoRaTP(loRaTP);
    frame->setBeaconTimer(beaconTimer);
    frame->setPingNb(pingNumber);
    //frame->setPingPeriod(pingPeriod);


    beacon->insertAtFront(frame);
    sendDown(beacon);


    //auto pkt = new Packet("DataFrame");
    //pkt->setKind(Beacon);
    //auto frame = makeShared<LoRaMacFrame>();
    //frame->setChunkLength(B(par("headerLength").intValue()));

    //lastSentMeasurement = rand();
    //payload->setSampleMeasurement(lastSentMeasurement);
    //const L3Address& gwAddress = "255.255.255.255";
    //auto pktAux = new Packet("HI I AM AN ACK MESSAGE !");
    EV<<"HELLO !!!!" << endl;
    //frame->setReceiverAddress(MacAddress::BROADCAST_ADDRESS);
    //pkt->insertAtFront(frame);
    //pktRequest->insertAtBack(payload);
    //sendDown(pkt);
    //send(pkt,outGate);
    //send(pktRequest, "upperMgmtOut");
    //socket.sendTo(pktAux, MacAddress::BROADCAST_ADDRESS, -1);
}

/*
void LoRaGWMac::createFakeLoRaMacFrame()
{

}
*/
/*
void LoRaGWMac::distanceCalculation(int devId){

    string pathToFiles = "/home/mehdy/LEO_";
    string endOfPathToFiles = "-LLA-Pos.csv";

    double lat_1;
    double lon_1;
    double alt_1;

    double lat_2;
    double lon_2;
    double alt_2;

    double lat_3;
    double lon_3;
    double alt_3;

    const double r = 6376.5; // Radius of Earth in Kilometres
    auto myGW = getContainingNode(this);

    //if(devId==4){

    ifstream me(pathToFiles+to_string(devId)+endOfPathToFiles);
    if(!me.is_open()) std::cout <<"ERROR: File Open" << '\n';

    string TIME;
    string LAT_1;
    string LON_1;
    string ALT_1;

    int i=1;

    while(i<=k){

    //while(ip.good()){
        getline(me,TIME,',');
        getline(me,LAT_1,',');
        getline(me,LON_1,',');
        getline(me,ALT_1,'\n');


        //std::cout<<"TIME: "<<TIME<< '\n';
        std::cout<<"Lattitude: "<<LAT_1<< '\n';
        //std::cout<<"Longitude: "<<LON_1<< '\n';
        //std::cout<<"Altitude: "<<ALT_1<< '\n';

        i++;
    }
    me.close();

    lat_1=strtod(LAT_1.c_str(),NULL);
    lon_1=strtod(LON_1.c_str(),NULL);
    alt_1=strtod(ALT_1.c_str(),NULL);

    printf("MY LATITUDE IS : %lf \n",lat_1);
    printf("MY LONGITUDE IS : %lf \n",lon_1);
    printf("MY ALTITUDE IS : %lf \n",alt_1);

    printf("-------------------------- \n");


    double x_1 = (r + alt_1) * sin(lon_1) * cos(lat_1);
    double y_1 = (r + alt_1) * sin(lon_1) * sin(lat_1);
    double z_1 = (r + alt_1) * cos(lon_1);
    int tmp1=devId;

    if(tmp1!=3){
        int tmp2=devId;
        //EV<<"MY ID IS !!!!!!!!!!!!!!!!!!!"<<devId<<endl;
        if(tmp2!=7){
            int tmp3=devId;
            if(tmp3!=11){
                int tmp4=devId;
                if(tmp4!=15){
    //EV<<"MY ID IS !!!!!!!!!!!!!!!!!!!"<<devId<<endl;
    ifstream upNeighbor(pathToFiles+to_string((devId+1))+endOfPathToFiles);
    if(!upNeighbor.is_open()) std::cout <<"ERROR: File Open" << '\n';

    string TIME2;
    string LAT_2;
    string LON_2;
    string ALT_2;

    i=1;

    while(i<=k){

    //while(ip.good()){
        getline(upNeighbor,TIME2,',');
        getline(upNeighbor,LAT_2,',');
        getline(upNeighbor,LON_2,',');
        getline(upNeighbor,ALT_2,'\n');


        //std::cout<<"TIME: "<<TIME2<< '\n';
        //std::cout<<"Lattitude: "<<LAT_2<< '\n';
        //std::cout<<"Longitude: "<<LON_2<< '\n';
        //std::cout<<"Altitude: "<<ALT_2<< '\n';

        i++;
    }

    upNeighbor.close();
    lat_2=atof(LAT_2.c_str());
    lon_2=atof(LON_2.c_str());
    alt_2=atof(ALT_2.c_str());


    printf("UP NEIGHBOR LATITUDE IS : %lf \n",lat_2);
    printf("UP NEIGHBOR LONGITUDE IS : %lf \n",lon_2);
    printf("UP NEIGHBOR ALTITUDE IS : %lf \n",alt_2);

    printf("-------------------------- \n");

    double x_2 = (r + alt_2) * sin(lon_2) * cos(lat_2);
    double y_2 = (r + alt_2) * sin(lon_2) * sin(lat_2);
    double z_2 = (r + alt_2) * cos(lon_2);

    double upDist = sqrt((x_2 - x_1) * (x_2 - x_1) + (y_2 - y_1) * (y_2 - y_1) + (z_2 - z_1) * (z_2 - z_1));

    printf("UP DISTANCE IS : %lf \n",upDist);
    printf("-------------------------- \n");

    myGW->par("retardUp")= (upDist / 200000);
    std::cout<<"DELAY TO UP NEIGHBOR IS : "<<myGW->par("retardUp")<< '\n';
    printf("-------------------------- \n");
    }
            }}}
    int tmp6=devId;
    if(tmp6<12){
    ifstream rightNeighbor(pathToFiles+to_string((devId+4))+endOfPathToFiles);
        if(!rightNeighbor.is_open()) std::cout <<"ERROR: File Open" << '\n';

    string TIME3;
    string LAT_3;
    string LON_3;
    string ALT_3;

    i=1;

    while(i<=k){

    //while(ip.good()){
        getline(rightNeighbor,TIME3,',');
        getline(rightNeighbor,LAT_3,',');
        getline(rightNeighbor,LON_3,',');
        getline(rightNeighbor,ALT_3,'\n');


            //std::cout<<"TIME: "<<TIME2<< '\n';
            //std::cout<<"Lattitude: "<<LAT_2<< '\n';
            //std::cout<<"Longitude: "<<LON_2<< '\n';
            //std::cout<<"Altitude: "<<ALT_2<< '\n';

        i++;
    }

    rightNeighbor.close();

    lat_3=atof(LAT_3.c_str());
    lon_3=atof(LON_3.c_str());
    alt_3=atof(ALT_3.c_str());

    printf("RIGHT NEIGHBOR LATITUDE IS : %lf \n",lat_3);
    printf("RIGHT NEIGHBOR LONGITUDE IS : %lf \n",lon_3);
    printf("RIGHT NEIGHBOR ALTITUDE IS : %lf \n",alt_3);

    printf("-------------------------- \n");

    double x_3 = (r + alt_3) * sin(lon_3) * cos(lat_3);
    double y_3 = (r + alt_3) * sin(lon_3) * sin(lat_3);
    double z_3 = (r + alt_3) * cos(lon_3);

    double rightDist = sqrt((x_3 - x_1) * (x_3 - x_1) + (y_3 - y_1) * (y_3 - y_1) + (z_3 - z_1) * (z_3 - z_1));

    printf("RIGHT DISTANCE IS : %lf \n",rightDist);
    printf("-------------------------- \n");

    myGW->par("retardRight")= (rightDist / 200000);

    std::cout<<"DELAY TO RIGHT NEIGHBOR IS : "<<myGW->par("retardRight")<< '\n';
    printf("-------------------------- \n");

    }
    //double lat_1 = pos.x;
    //double lon_1 = pos.y;
    //double alt_1 = ALT;

    //double lat_2 = 18.4581253333333 * (Math.PI / 180);
    //double lon_2 = 73.3963755277778 * (Math.PI / 180);
    //double alt_2 = 317.473;



    //std::cout<<"MY LATITUDE IS : "<<lat_1<< '\n';
    //std::cout<<"MY LONGITUDE IS : "<<lon_1<< '\n';
    //std::cout<<"MY ALTITUDE IS : "<<alt_1<< '\n';

    //std::cout<<"--------------------------"<<'\n';

    //std::cout<<"NEIGHBOR LATITUDE IS : "<<lat_2<< '\n';
    //std::cout<<"NEIGHBOR LONGITUDE IS : "<<lon_2<< '\n';
    //std::cout<<"NEIGHBOR ALTITUDE IS : "<<alt_2<< '\n';

    //satToSatDistance = pos.x


    //std::cout<<"UP DISTANCE IS : "<<upDist<< '\n';
    //std::cout<<"RIGHT DISTANCE IS : "<<rightDist<< '\n';


    //printf("DELAY TO UP NEIGHBOR IS : %lf \n",myGW->par("retardUp"));
    //printf("DELAY TO RIGHT NEIGHBOR IS : %lf \n",myGW->par("retardRight"));
    //EV<<"MY ID IS !!!!!!!!!!!!!!!!!!!"<<devId<<endl;
    k++;
    EV<<"THE DELAY BETWEEN SAT "<< devId <<" AND SAT "<<devId+1<< " IS APPROXIMATELY : "<<myGW->par("retardUp")<<endl;
    EV<<"THE DELAY BETWEEN SAT "<< devId <<" AND SAT "<<devId+4<< " IS APPROXIMATELY : "<<myGW->par("retardRight")<<endl;
    //EV<<"HELLOOOOOOOOOOOOOO !!"<<dist<<endl;
    //}
}*/
void LoRaGWMac::receiveSignal(cComponent *source, simsignal_t signalID, intval_t value, cObject *details)
{
    Enter_Method_Silent();
    if (signalID == IRadio::transmissionStateChangedSignal) {
        IRadio::TransmissionState newRadioTransmissionState = (IRadio::TransmissionState)value;
        if (transmissionState == IRadio::TRANSMISSION_STATE_TRANSMITTING && newRadioTransmissionState == IRadio::TRANSMISSION_STATE_IDLE) {
            //transmissin is finished
            radio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);
        }
        transmissionState = newRadioTransmissionState;
    }
}

MacAddress LoRaGWMac::getAddress()
{
    return address;
}

}
