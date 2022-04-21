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

#include "ISLPacketForwarder.h"
//#include "inet/networklayer/ipv4/IPv4Datagram.h"
//#include "inet/networklayer/contract/ipv4/IPv4ControlInfo.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/common/ModuleAccess.h"
#include "inet/applications/base/ApplicationPacket_m.h"
#include "../LoRaPhy/LoRaRadioControlInfo_m.h"
#include "inet/physicallayer/wireless/common/contract/packetlevel/SignalTag_m.h"

#include "LoRaGWMac.h"
#include "inet/common/ModuleAccess.h"
#include "../LoRaPhy/LoRaPhyPreamble_m.h"
#include "inet/common/ProtocolTag_m.h"
#include "LoRaApp/SimpleLoRaApp.h"
#include "ISLChannel.h"
#include "ISLPacketForwarder.h"
#include "inet/mobility/single/BonnMotionMobility.h"
#include <cmath>
#include <math.h>
#define PI 3.14159265

#include <string.h>
//#include <omnetpp.h>

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

Define_Module(ISLPacketForwarder);

//This module will route the UPLINK and DOWNLINK following basic routing protocol RIGHT then UP

void ISLPacketForwarder::initialize(int stage) {
    if (stage == 0) {
        LoRa_GWPacketReceived = registerSignal("LoRa_GWPacketReceived");
        localPort = par("localPort");
        destPort = par("destPort");
    } else if (stage == INITSTAGE_APPLICATION_LAYER) {
        updateISLDistance = new cMessage("Calculate the distance");
        sendRight = new cMessage("Sending Right");
        satelliteID = par("satIndex");
        updateISLDistanceInterval = par("updateISLDistanceInterval");
        scheduleAt(simTime(), updateISLDistance);
        //  startUDP();
        //  getSimulation()->getSystemModule()->subscribe("LoRa_AppPacketSent", this);
    }
}

void ISLPacketForwarder::startUDP() {
    EV << "We are breaking out here" << endl;
    socket.setOutputGate(gate("socketOut"));
    const char *localAddress = par("localAddress");
    socket.bind(
            *localAddress ?
                    L3AddressResolver().resolve(localAddress) : L3Address(),
            localPort);
    EV << "we arrived behind the first resolv" << endl;
    // TODO: is this required?
    //setSocketOptions();

    const char *destAddrs = par("destAddresses");
    cStringTokenizer tokenizer(destAddrs);
    const char *token;

    // Create UDP sockets to multiple destination addresses (network servers)
    while ((token = tokenizer.nextToken()) != nullptr) {
        EV << "into the loop" << endl;
        EV << token << endl;
        L3Address result;
        L3AddressResolver().tryResolve(token, result);
        EV << "out of the loop" << endl;
        if (result.isUnspecified())
            EV_ERROR << "cannot resolve destination address: " << token << endl;
        else
            EV << "Got destination address: " << token << endl;
        destAddresses.push_back(result);
    }
    EV << "We've made it to the end" << endl;
}

void ISLPacketForwarder::handleMessage(cMessage *msg) {
    auto myParentGW = getContainingNode(this);
    EV << msg->getArrivalGate() << endl;

    // Messaged arrived from the local satellite (LoRa or GS interfaces)
    if (msg->arrivedOn("satPart$i")) {

        // Insert local satelliteID in the frame header
        auto pkt = check_and_cast<Packet*>(msg);
        pkt->trimFront();
        auto frame = pkt->removeAtFront<LoRaMacFrame>();
        int oldSatID = frame->getSatNumber();
        frame->setSatNumber(satelliteID);
        pkt->insertAtFront(frame);

//        std::cout << "ISLPacketForwarder (sat" << satelliteID
//                << "): message arrived from local sat"
//                << std::endl;
    }

    // Update ISL distance every interval
    if (msg == updateISLDistance) {
        distanceCalculation(satelliteID);
        scheduleAt(simTime() + updateISLDistanceInterval, updateISLDistance);

    // Any data packet arriving to this module
    } else {
        auto pkt = check_and_cast<Packet*>(msg);
        pkt->trimFront();
        auto frame = pkt->removeAtFront<LoRaMacFrame>();

        // Packet should be forwarded to the ground station / network server
        if (frame->getPktType() == UPLINK) {

            // Gate to the neighbor on the right
            cGate *externalGateToRightNeighbor = myParentGW->gate("right$o");
            cGate *internalGateToRightNeighbor = externalGateToRightNeighbor->getPathStartGate();

            // Gate to the neighbor on the top
            cGate *externalGateToUpperNeighbor = myParentGW->gate("up$o");
            cGate *internalGateToUpperNeighbor = externalGateToUpperNeighbor->getPathStartGate();

            // If available, first forward to the right neighbor
            if (externalGateToRightNeighbor->isConnected()) {

                // Store path in frame
                frame->setNumHop(frame->getNumHop() + 1);
                frame->setTmpPath6(frame->getTmpPath5());
                frame->setTmpPath5(frame->getTmpPath4());
                frame->setTmpPath4(frame->getTmpPath3());
                frame->setTmpPath3(frame->getTmpPath2());
                frame->setTmpPath2(frame->getTmpPath1());
                frame->setTmpPath1(HORIZONTAL_HOP);
                pkt->insertAtFront(frame);

                // Enqueue packet towards the right neighbor
                if (internalGateToRightNeighbor->getTransmissionChannel()->isBusy() == false) {
                    send(pkt, internalGateToRightNeighbor);
                } else {
                    scheduleAt(internalGateToRightNeighbor->getTransmissionChannel()->getTransmissionFinishTime(), msg);
                }

            // If right not available, forward to the upper neighbor
            } else if (externalGateToUpperNeighbor->isConnected()) {

                // Store path in frame
                frame->setNumHop(frame->getNumHop() + 1);
                frame->setTmpPath6(frame->getTmpPath5());
                frame->setTmpPath5(frame->getTmpPath4());
                frame->setTmpPath4(frame->getTmpPath3());
                frame->setTmpPath3(frame->getTmpPath2());
                frame->setTmpPath2(frame->getTmpPath1());
                frame->setTmpPath1(VERTICAL_HOP);
                pkt->insertAtFront(frame);

                // Enqueue packet towards the upper neighbor
                if (internalGateToUpperNeighbor->getTransmissionChannel()->isBusy() == false) {
                    send(pkt, internalGateToUpperNeighbor);
                } else {
                    scheduleAt(internalGateToUpperNeighbor->getTransmissionChannel()->getTransmissionFinishTime(), msg);
                }

            // If right nor upper are available, forward to the ground station
            } else {
//                std::cout << "ISLPacketForwarder (sat" << satelliteID
//                        << "): forwarding message to GS"
//                        << std::endl;

                // Store path in frame (unchaged)
                frame->setNumHop(frame->getNumHop());
                frame->setTmpPath6(frame->getTmpPath6());
                frame->setTmpPath5(frame->getTmpPath5());
                frame->setTmpPath4(frame->getTmpPath4());
                frame->setTmpPath3(frame->getTmpPath3());
                frame->setTmpPath2(frame->getTmpPath2());
                frame->setTmpPath1(frame->getTmpPath1());
                pkt->insertAtFront(frame);

                send(pkt, "satPart$o");
            }

        // Packet should be forwarded to a LoRaWAN node
        } else if (frame->getPktType() == DOWNLINK) {

            // This hop is the local satellite, send to LoRaGWNic
            if (frame->getTmpPath1() == 0) {
                frame->setNumHop(frame->getNumHop());
                frame->setTmpPath6(frame->getTmpPath6());
                frame->setTmpPath5(frame->getTmpPath5());
                frame->setTmpPath4(frame->getTmpPath4());
                frame->setTmpPath3(frame->getTmpPath3());
                frame->setTmpPath2(frame->getTmpPath2());
                frame->setTmpPath1(frame->getTmpPath1());
                pkt->insertAtFront(frame);

                send(pkt, "satPart$o");

            // This packet is not for the local satellite, keep forwarding it
            } else {
                pkt->trimFront();

                // Forward to the bottom neighbor
                if (frame->getTmpPath1() == VERTICAL_HOP) {

                    // Update path
                    frame->setTmpPath1(frame->getTmpPath2());
                    frame->setTmpPath2(frame->getTmpPath3());
                    frame->setTmpPath3(frame->getTmpPath4());
                    frame->setTmpPath4(frame->getTmpPath5());
                    frame->setTmpPath5(frame->getTmpPath6());
                    frame->setTmpPath6(0); // last is local hop
                    pkt->insertAtFront(frame);

                    cGate *externalGateToBottomNeighbor = myParentGW->gate("down$o");
                    cGate *internalGateToBottomNeighbor = externalGateToBottomNeighbor->getPathStartGate();

                    if (externalGateToBottomNeighbor->isConnected()) {

                        if (internalGateToBottomNeighbor->getTransmissionChannel()->isBusy() == false) {
                            send(pkt, internalGateToBottomNeighbor);
                        } else {
                            scheduleAt(internalGateToBottomNeighbor->getTransmissionChannel()->getTransmissionFinishTime(), msg);
                        }
                    }

                // Forward to the left neighbor
                } else if (frame->getTmpPath1() == HORIZONTAL_HOP) {

                    // Update path
                    frame->setTmpPath1(frame->getTmpPath2());
                    frame->setTmpPath2(frame->getTmpPath3());
                    frame->setTmpPath3(frame->getTmpPath4());
                    frame->setTmpPath4(frame->getTmpPath5());
                    frame->setTmpPath5(frame->getTmpPath6());
                    frame->setTmpPath6(0);  // last is local hop
                    pkt->insertAtFront(frame);

                    cGate *externalGateToLeftNeighbor = myParentGW->gate("left$o");
                    cGate *internalGateToLeftNeighbor = externalGateToLeftNeighbor->getPathStartGate();

                    if (externalGateToLeftNeighbor->isConnected()) {

                        if (internalGateToLeftNeighbor->getTransmissionChannel()->isBusy() == false) {
                            send(pkt, internalGateToLeftNeighbor);
                        } else {
                            scheduleAt(internalGateToLeftNeighbor->getTransmissionChannel()->getTransmissionFinishTime(), msg);
                        }
                    }
                }
            }
        }
    }
}

//this function will help in updating ISL datarate in real time
void ISLPacketForwarder::distanceCalculation(int devId) {

    string pathToFiles = "files/LEO_";
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

    const double r = 6371; // Radius of Earth in Kilometres
    auto myGW = getContainingNode(this);

    //if(devId==4){

    ifstream me(
            pathToFiles + to_string(devId / 4) + "_" + to_string(devId % 4)
                    + endOfPathToFiles);
    if (!me.is_open())
        std::cout << "ERROR: File Open "
                << pathToFiles + to_string(devId / 4) + "_"
                        + to_string(devId % 4) + endOfPathToFiles << '\n';

    string TIME;
    string LAT_1;
    string LON_1;
    string ALT_1;

    int i = 1;

    while (i <= k) {

        //while(ip.good()){
        getline(me, TIME, ',');
        getline(me, LAT_1, ',');
        getline(me, LON_1, ',');
        getline(me, ALT_1, '\n');

        //std::cout<<"TIME: "<<TIME<< '\n';
        //std::cout<<"Lattitude: "<<LAT_1<< '\n';
        //std::cout<<"Longitude: "<<LON_1<< '\n';
        //std::cout<<"Altitude: "<<ALT_1<< '\n';

        i++;
    }
    me.close();

    lat_1 = strtod(LAT_1.c_str(), NULL);
    lon_1 = strtod(LON_1.c_str(), NULL);
    alt_1 = strtod(ALT_1.c_str(), NULL);

    //printf("MY LATITUDE IS : %lf \n",lat_1);
    //printf("MY LONGITUDE IS : %lf \n",lon_1);
    //printf("MY ALTITUDE IS : %lf \n",alt_1);

    //printf("-------------------------- \n");

    double x_1 = (r + alt_1) * sin(lon_1) * cos(lat_1);
    double y_1 = (r + alt_1) * sin(lon_1) * sin(lat_1);
    double z_1 = (r + alt_1) * cos(lon_1);
    int tmp1 = devId;

    if (tmp1 != 3) {
        int tmp2 = devId;
        //EV<<"MY ID IS !!!!!!!!!!!!!!!!!!!"<<devId<<endl;
        if (tmp2 != 7) {
            int tmp3 = devId;
            if (tmp3 != 11) {
                int tmp4 = devId;
                if (tmp4 != 15) {
                    //EV<<"MY ID IS !!!!!!!!!!!!!!!!!!!"<<devId<<endl;
                    ifstream upNeighbor(
                            pathToFiles + to_string((devId + 1) / 4) + "_"
                                    + to_string((devId + 1) % 4)
                                    + endOfPathToFiles);
                    if (!upNeighbor.is_open())
                        std::cout << "ERROR: File Open "
                                << pathToFiles + to_string((devId + 1) / 4)
                                        + "_" + to_string((devId + 1) % 4)
                                        + endOfPathToFiles << '\n';

                    string TIME2;
                    string LAT_2;
                    string LON_2;
                    string ALT_2;

                    i = 1;

                    while (i <= k) {

                        //while(ip.good()){
                        getline(upNeighbor, TIME2, ',');
                        getline(upNeighbor, LAT_2, ',');
                        getline(upNeighbor, LON_2, ',');
                        getline(upNeighbor, ALT_2, '\n');

                        //std::cout<<"TIME: "<<TIME2<< '\n';
                        //std::cout<<"Lattitude: "<<LAT_2<< '\n';
                        //std::cout<<"Longitude: "<<LON_2<< '\n';
                        //std::cout<<"Altitude: "<<ALT_2<< '\n';

                        i++;
                    }

                    upNeighbor.close();
                    lat_2 = atof(LAT_2.c_str());
                    lon_2 = atof(LON_2.c_str());
                    alt_2 = atof(ALT_2.c_str());

                    //printf("UP NEIGHBOR LATITUDE IS : %lf \n",lat_2);
                    //printf("UP NEIGHBOR LONGITUDE IS : %lf \n",lon_2);
                    //printf("UP NEIGHBOR ALTITUDE IS : %lf \n",alt_2);

                    //printf("-------------------------- \n");

                    double x_2 = (r + alt_2) * sin(lon_2) * cos(lat_2);
                    double y_2 = (r + alt_2) * sin(lon_2) * sin(lat_2);
                    double z_2 = (r + alt_2) * cos(lon_2);

                    double phi1 = lat_1 * (PI / 180);
                    double phi2 = lat_2 * (PI / 180);
                    double deltaPhi = (phi2 - phi1) * (PI / 180);
                    double deltaLambda = (lon_2 - lon_1) * (PI / 180);
                    double a = (sin((deltaPhi) / 2) * sin((deltaPhi) / 2))
                            + (cos(phi1) * cos(phi2)
                                    * (sin((deltaLambda) / 2)
                                            * sin((deltaLambda) / 2)));
                    double c = 2 * (atan2((sqrt(a)), (sqrt(1 - a))));
                    double d = r * c;
                    double d9 =
                            acos(
                                    (sin(phi1) * sin(phi2))
                                            + (cos(phi1) * cos(phi2)
                                                    * cos(deltaLambda))) * r;
                    //double upDist = d9;
                    double upDist = sqrt(
                            (d9 * d9) + ((alt_2 - alt_1) * (alt_2 - alt_1)));
                    //double upDist = sqrt((x_2 - x_1) * (x_2 - x_1) + (y_2 - y_1) * (y_2 - y_1) + (z_2 - z_1) * (z_2 - z_1));

                    //printf("UP DISTANCE IS : %lf \n",upDist);
                    //printf("-------------------------- \n");

                    myGW->par("retardUp") = (upDist / 200000);
                    cGate *upGatei = myGW->gate("up$i");
                    cDatarateChannel *upChanneli =
                            (cDatarateChannel*) upGatei->getIncomingTransmissionChannel();
                    upChanneli->setDelay(upDist / 200000);

                    cGate *upGateo = myGW->gate("up$o");
                    cDatarateChannel *upChannelo =
                            (cDatarateChannel*) upGateo->getTransmissionChannel();
                    upChannelo->setDelay(upDist / 200000);
                    /*
                     cGate* downGatei = myGW->gate("down$i");
                     cDatarateChannel* downChanneli = (cDatarateChannel*)downGatei->getIncomingTransmissionChannel();
                     downChanneli->setDelay(upDist/200000);

                     cGate* downGateo = myGW->gate("down$o");
                     cDatarateChannel* downChannelo = (cDatarateChannel*)downGateo->getTransmissionChannel();
                     downChannelo->setDelay(upDist/200000);
                     */
                    //std::cout<<"DELAY TO UP NEIGHBOR IS : "<<myGW->par("retardUp")<< '\n';
                    //printf("-------------------------- \n");
                }
            }
        }
    }
    int tmp6 = devId;
    if (tmp6 < 12) {
        ifstream rightNeighbor(
                pathToFiles + to_string((devId + 4) / 4) + "_"
                        + to_string((devId + 4) % 4) + endOfPathToFiles);
        if (!rightNeighbor.is_open())
            std::cout << "ERROR: File Open "
                    << pathToFiles + to_string((devId + 4) / 4) + "_"
                            + to_string((devId + 4) % 4) + endOfPathToFiles
                    << '\n';

        string TIME3;
        string LAT_3;
        string LON_3;
        string ALT_3;

        i = 1;

        while (i <= k) {

            //while(ip.good()){
            getline(rightNeighbor, TIME3, ',');
            getline(rightNeighbor, LAT_3, ',');
            getline(rightNeighbor, LON_3, ',');
            getline(rightNeighbor, ALT_3, '\n');

            //std::cout<<"TIME: "<<TIME2<< '\n';
            //std::cout<<"Lattitude: "<<LAT_2<< '\n';
            //std::cout<<"Longitude: "<<LON_2<< '\n';
            //std::cout<<"Altitude: "<<ALT_2<< '\n';

            i++;
        }

        rightNeighbor.close();

        lat_3 = atof(LAT_3.c_str());
        lon_3 = atof(LON_3.c_str());
        alt_3 = atof(ALT_3.c_str());

        //printf("RIGHT NEIGHBOR LATITUDE IS : %lf \n",lat_3);
        //printf("RIGHT NEIGHBOR LONGITUDE IS : %lf \n",lon_3);
        //printf("RIGHT NEIGHBOR ALTITUDE IS : %lf \n",alt_3);

        //printf("-------------------------- \n");

        double x_3 = (r + alt_3) * sin(lon_3) * cos(lat_3);
        double y_3 = (r + alt_3) * sin(lon_3) * sin(lat_3);
        double z_3 = (r + alt_3) * cos(lon_3);

        double phi1 = lat_1 * (PI / 180);
        double phi3 = lat_3 * (PI / 180);
        double deltaPhi2 = (phi3 - phi1) * (PI / 180);
        double deltaLambda2 = (lon_3 - lon_1) * (PI / 180);
        double a2 = (sin((deltaPhi2) / 2) * sin((deltaPhi2) / 2))
                + (cos(phi1) * cos(phi3)
                        * (sin((deltaLambda2) / 2) * sin((deltaLambda2) / 2)));
        double c2 = 2 * (atan2((sqrt(a2)), (sqrt(1 - a2))));
        double d2 = r * c2;
        double d8 = acos(
                (sin(phi1) * sin(phi3))
                        + (cos(phi1) * cos(phi3) * cos(deltaLambda2))) * r;
        //double rightDist=d8;
        double rightDist = sqrt(
                (d8 * d8) + ((alt_3 - alt_1) * (alt_3 - alt_1)));
        //double rightDist = sqrt((x_3 - x_1) * (x_3 - x_1) + (y_3 - y_1) * (y_3 - y_1) + (z_3 - z_1) * (z_3 - z_1));

        //printf("RIGHT DISTANCE IS : %lf \n",rightDist);
        //printf("-------------------------- \n");

        myGW->par("retardRight") = (rightDist / 200000);

        cGate *upGateir = myGW->gate("right$i");
        cDatarateChannel *upChannelir =
                (cDatarateChannel*) upGateir->getIncomingTransmissionChannel();
        upChannelir->setDelay(rightDist / 200000);

        cGate *upGateor = myGW->gate("right$o");
        cDatarateChannel *upChannelor =
                (cDatarateChannel*) upGateor->getTransmissionChannel();
        upChannelor->setDelay(rightDist / 200000);
        /*
         cGate* upGateil = myGW->gate("left$i");
         cDatarateChannel* upChannelil = (cDatarateChannel*)upGateil->getIncomingTransmissionChannel();
         upChannelil->setDelay(rightDist/200000);

         cGate* upGateol = myGW->gate("left$o");
         cDatarateChannel* upChannelol = (cDatarateChannel*)upGateol->getTransmissionChannel();
         upChannelol->setDelay(rightDist/200000);
         */
        //std::cout<<"DELAY TO RIGHT NEIGHBOR IS : "<<myGW->par("retardRight")<< '\n';
        //printf("-------------------------- \n");
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

    //EV<<"THE DELAY BETWEEN SAT "<< devId <<" AND SAT "<<devId+1<< " IS APPROXIMATELY : "<<myGW->par("retardUp")<<endl;
    //EV<<"THE DELAY BETWEEN SAT "<< devId <<" AND SAT "<<devId+4<< " IS APPROXIMATELY : "<<myGW->par("retardRight")<<endl;
    //EV<<"HELLOOOOOOOOOOOOOO !!"<<dist<<endl;
    //}
}

void ISLPacketForwarder::finish() {
    recordScalar("LoRa_GW_DER",
            double(counterOfReceivedPackets) / counterOfSentPacketsFromNodes);
    cancelAndDelete(updateISLDistance);
}

} //namespace inet
