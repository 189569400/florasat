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
#define PI 3.14159265
#include "NetworkServerApp.h"
//#include "inet/networklayer/ipv4/IPv4Datagram.h"
//#include "inet/networklayer/contract/ipv4/IPv4ControlInfo.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/transportlayer/common/L4PortTag_m.h"
#include "inet/transportlayer/contract/udp/UdpControlInfo_m.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/common/ModuleAccess.h"
#include "inet/applications/base/ApplicationPacket_m.h"

#include "inet/networklayer/common/L3Tools.h"
#include "inet/networklayer/ipv4/Ipv4Header_m.h"

#include<fstream>
#include <cmath>
#include <iostream>
#include <string.h>
#include <math.h>

#include <cstring>
#include <sstream>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

using namespace std;

namespace flora {

Define_Module(NetworkServerApp);


void NetworkServerApp::initialize(int stage)
{
    if (stage == 0) {
        ASSERT(recvdPackets.size()==0);
        LoRa_ServerPacketReceived = registerSignal("LoRa_ServerPacketReceived");
        localPort = par("localPort");
        destPort = par("destPort");
        adrMethod = par("adrMethod").stdstringValue();
    } else if (stage == INITSTAGE_APPLICATION_LAYER) {
        startUDP();
        getSimulation()->getSystemModule()->subscribe("LoRa_AppPacketSent", this);
        evaluateADRinServer = par("evaluateADRinServer");
        WorkWithAck = par("WorkWithAck");
        adrDeviceMargin = par("adrDeviceMargin");
        receivedRSSI.setName("Received RSSI");
        totalReceivedPackets = 0;
        totalCase1 = 0;
        totalCase2 = 0;
        totalCase3 = 0;
        totalCase4 = 0;
        netServerDPAD = 0;
        dpadVector.setName("DPAD Vector");
        WATCH(netServerDPAD);
        calculationYay = new cMessage("it is Time to calculate delay !");
        scheduleAt(simTime()+calculationDelay,calculationYay);
        //BeaconTimer = new cMessage("Beacon Timer");
        //scheduleAt(simTime() + 1, BeaconTimer);
        for(int i=0;i<6;i++)
        {
            counterUniqueReceivedPacketsPerSF[i] = 0;
            counterOfSentPacketsFromNodesPerSF[i] = 0;
        }
    }
}


void NetworkServerApp::startUDP()
{
    socket.setOutputGate(gate("socketOut"));
    const char *localAddress = par("localAddress");
    socket.bind(*localAddress ? L3AddressResolver().resolve(localAddress) : L3Address(), localPort);
}


void NetworkServerApp::handleMessage(cMessage *msg)
{
    if (msg->arrivedOn("socketIn")) {
        auto pkt = check_and_cast<Packet *>(msg);
        const auto &frame  = pkt->peekAtFront<LoRaMacFrame>();
        if (frame == nullptr)
            throw cRuntimeError("Header error type");
        //LoRaMacFrame *frame = check_and_cast<LoRaMacFrame *>(msg);
        if (simTime() >= getSimulation()->getWarmupPeriod())
        {
            totalReceivedPackets++;
        }
        updateKnownNodes(pkt);
        processLoraMACPacket(pkt);

    }
    else if(msg->isSelfMessage()) {
        //Used to set a table of all the propagation delay between ISL
        if(msg==calculationYay){
            calculateDelayTime();
            scheduleAt(simTime()+calculationDelay,calculationYay);
            EV<<"Sat 0 To Sat 1 => "<<pathToRightGW[0]<<" s"<<endl;
            EV<<"Sat 0 To Sat 4 => "<<pathToRightGW[1]<<" s"<<endl;
            EV<<"Sat 1 To Sat 2 => "<<pathToRightGW[2]<<" s"<<endl;
            EV<<"Sat 1 To Sat 5 => "<<pathToRightGW[3]<<" s"<<endl;
            EV<<"Sat 2 To Sat 3 => "<<pathToRightGW[4]<<" s"<<endl;
            EV<<"Sat 2 To Sat 6 => "<<pathToRightGW[5]<<" s"<<endl;
            EV<<"Sat 3 To Sat 7 => "<<pathToRightGW[6]<<" s"<<endl;
            EV<<"Sat 4 To Sat 5 => "<<pathToRightGW[7]<<" s"<<endl;
            EV<<"Sat 4 To Sat 8 => "<<pathToRightGW[8]<<" s"<<endl;
            EV<<"Sat 5 To Sat 6 => "<<pathToRightGW[9]<<" s"<<endl;
            EV<<"Sat 5 To Sat 9 => "<<pathToRightGW[10]<<" s"<<endl;
            EV<<"Sat 6 To Sat 7 => "<<pathToRightGW[11]<<" s"<<endl;
            EV<<"Sat 6 To Sat 10 => "<<pathToRightGW[12]<<" s"<<endl;
            EV<<"Sat 7 To Sat 11 => "<<pathToRightGW[13]<<" s"<<endl;
            EV<<"Sat 8 To Sat 9 => "<<pathToRightGW[14]<<" s"<<endl;
            EV<<"Sat 8 To Sat 12 => "<<pathToRightGW[15]<<" s"<<endl;
            EV<<"Sat 9 To Sat 10 => "<<pathToRightGW[16]<<" s"<<endl;
            EV<<"Sat 9 To Sat 13 => "<<pathToRightGW[17]<<" s"<<endl;
            EV<<"Sat 10 To Sat 11 => "<<pathToRightGW[18]<<" s"<<endl;
            EV<<"Sat 10 To Sat 14 => "<<pathToRightGW[19]<<" s"<<endl;
            EV<<"Sat 11 To Sat 15 => "<<pathToRightGW[20]<<" s"<<endl;
            EV<<"Sat 12 To Sat 13 => "<<pathToRightGW[21]<<" s"<<endl;
            EV<<"Sat 13 To Sat 14 => "<<pathToRightGW[22]<<" s"<<endl;
            EV<<"Sat 14 To Sat 15 => "<<pathToRightGW[23]<<" s"<<endl;
            //std::cout<<"THE TABLE OF PATH DELAY IS "<<pathToRightGW[0]<<'\n';
        }
      //  if(msg==BeaconTimer){
        //    EV<<"WELL DONE !!"<<endl;
            //sendBeacon();
        //}
        else{
        processScheduledPacket(msg);
        }
    }
}
/*
void NetworkServerApp::sendBeacon()
{
    //L3Address pickedGateway;
    //const L3Address& gwAddress;

    auto mgmtPacket = makeShared<LoRaAppPacket>();
    mgmtPacket->setMsgType(Beacon);
    auto frameToSend = makeShared<LoRaMacFrame>();
    frameToSend->setChunkLength(B(par("headerLength").intValue()));
         //  LoRaMacFrame *frameToSend = new LoRaMacFrame("ADRPacket");

                            //frameToSend->encapsulate(mgmtPacket);
    frameToSend->setReceiverAddress(MacAddress::BROADCAST_ADDRESS);
                        //FIXME: What value to set for LoRa TP
                            //frameToSend->setLoRaTP(pkt->getLoRaTP());
                            //frameToSend->setLoRaTP(frame->getLoRaTP());!!!!
    frameToSend->setLoRaTP(math::dBmW2mW(14));
    //frameToSend->setLoRaCF(frame->getLoRaCF());
    //frameToSend->setLoRaSF(frame->getLoRaSF());
    //frameToSend->setLoRaBW(frame->getLoRaBW());
    auto pktAux = new Packet("HI I COUNT TO 128 ! ");
    mgmtPacket->setChunkLength(B(par("headerLength").intValue()));

    pktAux->insertAtFront(mgmtPacket);
    pktAux->insertAtFront(frameToSend);
    socket.sendTo(pktAux, , destPort);


/*
    auto pktRequest = new Packet("DataFrame");
    pktRequest->setKind(Beacon);
    auto payload = makeShared<LoRaAppPacket>();
    payload->setChunkLength(B(par("dataSize").intValue()));

    //lastSentMeasurement = rand();
    //payload->setSampleMeasurement(lastSentMeasurement);
    //const L3Address& gwAddress = "255.255.255.255";
    //auto pktAux = new Packet("HI I AM AN ACK MESSAGE !");
    EV<<"HELLO !!!!" << endl;
    pktRequest->insertAtBack(payload);
    sendDown(pktRequest);
    //send(pktRequest, "upperMgmtOut");
    //socket.sendTo(pktAux, MacAddress::BROADCAST_ADDRESS, -1);
*/
//}

void NetworkServerApp::processLoraMACPacket(Packet *pk)
{
    const auto & frame = pk->peekAtFront<LoRaMacFrame>();
    if(isPacketProcessed(frame))
    {
        delete pk;
        return;
    }
    addPktToProcessingTable(pk);
}

void NetworkServerApp::finish()
{
    recordScalar("LoRa_NS_DER", double(counterUniqueReceivedPackets)/counterOfSentPacketsFromNodes);
    recordScalar("netServerDPAD", netServerDPAD);
    for(uint i=0;i<knownNodes.size();i++)
    {
        delete knownNodes[i].historyAllSNIR;
        delete knownNodes[i].historyAllRSSI;
        delete knownNodes[i].receivedSeqNumber;
        delete knownNodes[i].calculatedSNRmargin;
        recordScalar("Send ADR for node", knownNodes[i].numberOfSentADRPackets);
    }
    for (std::map<int,int>::iterator it=numReceivedPerNode.begin(); it != numReceivedPerNode.end(); ++it)
    {
        const std::string stringScalar = "numReceivedFromNode " + std::to_string(it->first);
        recordScalar(stringScalar.c_str(), it->second);
    }

    receivedRSSI.recordAs("receivedRSSI");
    recordScalar("totalReceivedPackets", totalReceivedPackets);
    recordScalar("totalCase1", totalCase1);
    recordScalar("totalCase2", totalCase2);
    recordScalar("totalCase3", totalCase3);
    recordScalar("totalCase4", totalCase4);

    while(!receivedPackets.empty()) {
        receivedPackets.back().endOfWaiting->removeControlInfo();
        delete receivedPackets.back().rcvdPacket;
        if (receivedPackets.back().endOfWaiting && receivedPackets.back().endOfWaiting->isScheduled()) {
            cancelAndDelete(receivedPackets.back().endOfWaiting);
        }
        else
            delete receivedPackets.back().endOfWaiting;
        receivedPackets.pop_back();
    }

    knownNodes.clear();
    receivedPackets.clear();

    recordScalar("counterUniqueReceivedPacketsPerSF SF7", counterUniqueReceivedPacketsPerSF[0]);
    recordScalar("counterUniqueReceivedPacketsPerSF SF8", counterUniqueReceivedPacketsPerSF[1]);
    recordScalar("counterUniqueReceivedPacketsPerSF SF9", counterUniqueReceivedPacketsPerSF[2]);
    recordScalar("counterUniqueReceivedPacketsPerSF SF10", counterUniqueReceivedPacketsPerSF[3]);
    recordScalar("counterUniqueReceivedPacketsPerSF SF11", counterUniqueReceivedPacketsPerSF[4]);
    recordScalar("counterUniqueReceivedPacketsPerSF SF12", counterUniqueReceivedPacketsPerSF[5]);
    if (counterOfSentPacketsFromNodesPerSF[0] > 0)
        recordScalar("DER SF7", double(counterUniqueReceivedPacketsPerSF[0]) / counterOfSentPacketsFromNodesPerSF[0]);
    else
        recordScalar("DER SF7", 0);

    if (counterOfSentPacketsFromNodesPerSF[1] > 0)
        recordScalar("DER SF8", double(counterUniqueReceivedPacketsPerSF[1]) / counterOfSentPacketsFromNodesPerSF[1]);
    else
        recordScalar("DER SF8", 0);

    if (counterOfSentPacketsFromNodesPerSF[2] > 0)
        recordScalar("DER SF9", double(counterUniqueReceivedPacketsPerSF[2]) / counterOfSentPacketsFromNodesPerSF[2]);
    else
        recordScalar("DER SF9", 0);

    if (counterOfSentPacketsFromNodesPerSF[3] > 0)
        recordScalar("DER SF10", double(counterUniqueReceivedPacketsPerSF[3]) / counterOfSentPacketsFromNodesPerSF[3]);
    else
        recordScalar("DER SF10", 0);

    if (counterOfSentPacketsFromNodesPerSF[4] > 0)
        recordScalar("DER SF11", double(counterUniqueReceivedPacketsPerSF[4]) / counterOfSentPacketsFromNodesPerSF[4]);
    else
        recordScalar("DER SF11", 0);

    if (counterOfSentPacketsFromNodesPerSF[5] > 0)
        recordScalar("DER SF12", double(counterUniqueReceivedPacketsPerSF[5]) / counterOfSentPacketsFromNodesPerSF[5]);
    else
        recordScalar("DER SF12", 0);
}

bool NetworkServerApp::isPacketProcessed(const Ptr<const LoRaMacFrame> &pkt)
{
    for(const auto & elem : knownNodes) {
        if(elem.srcAddr == pkt->getTransmitterAddress()) {
            if(elem.lastSeqNoProcessed > pkt->getSequenceNumber()) return true;
        }
    }
    return false;
}

void NetworkServerApp::updateKnownNodes(Packet* pkt)
{
    const auto & frame = pkt->peekAtFront<LoRaMacFrame>();
    bool nodeExist = false;
    for(auto &elem : knownNodes)
    {
        if(elem.srcAddr == frame->getTransmitterAddress()) {
            nodeExist = true;
            if(elem.lastSeqNoProcessed < frame->getSequenceNumber()) {
                elem.lastSeqNoProcessed = frame->getSequenceNumber();
            }
            break;
        }
    }

    if(nodeExist == false)
    {
        knownNode newNode;
        newNode.srcAddr= frame->getTransmitterAddress();
        newNode.lastSeqNoProcessed = frame->getSequenceNumber();
        newNode.framesFromLastADRCommand = 0;
        newNode.numberOfSentADRPackets = 0;
        newNode.historyAllSNIR = new cOutVector;
        newNode.historyAllSNIR->setName("Vector of SNIR per node");
        //newNode.historyAllSNIR->record(pkt->getSNIR());
        newNode.historyAllSNIR->record(math::fraction2dB(frame->getSNIR()));
        newNode.historyAllRSSI = new cOutVector;
        newNode.historyAllRSSI->setName("Vector of RSSI per node");
        newNode.historyAllRSSI->record(frame->getRSSI());
        newNode.receivedSeqNumber = new cOutVector;
        newNode.receivedSeqNumber->setName("Received Sequence number");
        newNode.calculatedSNRmargin = new cOutVector;
        newNode.calculatedSNRmargin->setName("Calculated SNRmargin in ADR");
        knownNodes.push_back(newNode);
    }
}

void NetworkServerApp::addPktToProcessingTable(Packet* pkt)
{
    const auto & frame = pkt->peekAtFront<LoRaMacFrame>();
    bool packetExists = false;
    for(auto &elem : receivedPackets)
    {
        const auto &frameAux = elem.rcvdPacket->peekAtFront<LoRaMacFrame>();
        if(frameAux->getTransmitterAddress() == frame->getTransmitterAddress() && frameAux->getSequenceNumber() == frame->getSequenceNumber())
        {
            packetExists = true;
            const auto& networkHeader = getNetworkProtocolHeader(pkt);
            const L3Address& gwAddress = networkHeader->getSourceAddress();
            elem.possibleGateways.emplace_back(gwAddress, math::fraction2dB(frame->getSNIR()), frame->getRSSI());
            delete pkt;
            break;
        }
    }
    if(packetExists == false)
    {
        receivedPacket rcvPkt;
        rcvPkt.rcvdPacket = pkt;
        rcvPkt.endOfWaiting = new cMessage("endOfWaitingWindow");
        rcvPkt.endOfWaiting->setControlInfo(pkt);
        const auto& networkHeader = getNetworkProtocolHeader(pkt);
        const L3Address& gwAddress = networkHeader->getSourceAddress();
        rcvPkt.possibleGateways.emplace_back(gwAddress, math::fraction2dB(frame->getSNIR()), frame->getRSSI());
        EV << "Added " << gwAddress << " " << math::fraction2dB(frame->getSNIR()) << " " << frame->getRSSI() << endl;
        scheduleAt(simTime() + 1, rcvPkt.endOfWaiting);
        receivedPackets.push_back(rcvPkt);
    }
}

void NetworkServerApp::processScheduledPacket(cMessage* selfMsg)
{
    auto pkt = check_and_cast<Packet *>(selfMsg->removeControlInfo());
    const auto & frame = pkt->peekAtFront<LoRaMacFrame>();

    if (simTime() >= getSimulation()->getWarmupPeriod())
    {
        counterUniqueReceivedPacketsPerSF[frame->getLoRaSF()-7]++;
    }
    L3Address pickedGateway;
    double SNIRinGW = -99999999999;
    double RSSIinGW = -99999999999;
    int packetNumber;
    int nodeNumber;
    for(uint i=0;i<receivedPackets.size();i++)
    {
        const auto &frameAux = receivedPackets[i].rcvdPacket->peekAtFront<LoRaMacFrame>();
        if(frameAux->getTransmitterAddress() == frame->getTransmitterAddress() && frameAux->getSequenceNumber() == frame->getSequenceNumber())        {
            packetNumber = i;
            nodeNumber = frame->getTransmitterAddress().getInt();
            if (numReceivedPerNode.count(nodeNumber-1)>0)
            {
                ++numReceivedPerNode[nodeNumber-1];
            } else {
                numReceivedPerNode[nodeNumber-1] = 1;
            }

            for(uint j=0;j<receivedPackets[i].possibleGateways.size();j++)
            {
                if(SNIRinGW < std::get<1>(receivedPackets[i].possibleGateways[j]))
                {
                    RSSIinGW = std::get<2>(receivedPackets[i].possibleGateways[j]);
                    SNIRinGW = std::get<1>(receivedPackets[i].possibleGateways[j]);
                    pickedGateway = std::get<0>(receivedPackets[i].possibleGateways[j]);
                }
            }
        }
    }
    emit(LoRa_ServerPacketReceived, true);
    if (simTime() >= getSimulation()->getWarmupPeriod())
    {
        counterUniqueReceivedPackets++;
    }
    receivedRSSI.collect(frame->getRSSI());
    if(WorkWithAck){
        int pathy[6]; //keep track of the actual routing path to follow
        pathy[0]=frame->getTmpPath1();
        pathy[1]=frame->getTmpPath2();
        pathy[2]=frame->getTmpPath3();
        pathy[3]=frame->getTmpPath4();
        pathy[4]=frame->getTmpPath5();
        pathy[5]=frame->getTmpPath6();
        //new ordered table to keep track of ISL propagation time
        followMe[0]=pathToRightGW[23];
        followMe[1]=pathToRightGW[22];
        followMe[2]=pathToRightGW[21];
        followMe[3]=pathToRightGW[18];
        followMe[4]=pathToRightGW[16];
        followMe[5]=pathToRightGW[14];
        followMe[6]=pathToRightGW[11];
        followMe[7]=pathToRightGW[9];
        followMe[8]=pathToRightGW[7];
        followMe[9]=pathToRightGW[4];
        followMe[10]=pathToRightGW[2];
        followMe[11]=pathToRightGW[0];
        followMe[12]=pathToRightGW[20];
        followMe[13]=pathToRightGW[13];
        followMe[14]=pathToRightGW[6];
        followMe[15]=pathToRightGW[19];
        followMe[16]=pathToRightGW[12];
        followMe[17]=pathToRightGW[5];
        followMe[18]=pathToRightGW[17];
        followMe[19]=pathToRightGW[10];
        followMe[20]=pathToRightGW[3];
        followMe[21]=pathToRightGW[15];
        followMe[22]=pathToRightGW[8];
        followMe[23]=pathToRightGW[1];
        double timeToArrive = 0;
        int i=0;
        int j=0;
        int l=12;
        for (i=0;i<6;i++){
            if (pathy[i]==2){
                timeToArrive = timeToArrive + followMe[j];
                j++;
                l=l+3;
            }
            else if(pathy[i]==1){
                timeToArrive = timeToArrive + followMe[l];
                j=j+3;
                l++;
            }
        }
        EV<<"THIS IS THE TOTAL ISL PROPAGATION TIME "<<timeToArrive; //d2(t2)

        auto myAddress = frame->getTransmitterAddress();
        int numberOfTheSat = frame->getSatNumber(); //get the sat device ID
        int numberOfTheNode = myAddress.getAddressByte(5); //get the node last value on IP 'nodes are ordered in IP address'

        double dist = satToNodeDelayTime(numberOfTheSat, numberOfTheNode); //calculate propagation time d1(t1) using Sat ID and node IP

        //auto myActualGW

        double nodeToSat = 2*(dist/300000);
        int hophop = frame->getNumHop(); //number of hop
        double sizeUpDown = (27*8)+(22*8);///10000 //total message size UL + DL
        double timeUpDown = sizeUpDown/20000; //total time on one ISL for message UL + DL SET THE DATARATE HERE
        double roadTime = hophop*timeUpDown; //ISL transmission time
        double islTime = 2*(timeToArrive); //total ISL propagation time UL + DL
        double totalTime = 0.024 + nodeToSat + islTime + roadTime; //total time 0.004 is sat15 to gwRouter
        //0.02 due to extra steps on the simulator between gwrouter and NS that i didn't get in it in details and just set measured it from simulation
        double t1 = totalTime + 1.2; // we have 1.2 wait delay on the NS before sending the DL
        EV<<"Number of hop "<<hophop<<endl;
        EV<<"Total size of the data "<<sizeUpDown<<endl;
        //EV<<"Time for UL+DL "<<timeUpDown<<endl;
        EV<<"Total Propagation time between Node and sat "<<nodeToSat<<endl;
        EV<<"Time for UL+DL on ISL "<<roadTime<<endl;
        EV<<"Total propagation time on ISL "<<islTime<<endl;
        EV<<"THE Network server DPAD is : "<<totalTime<<endl;
        netServerDPAD = totalTime;
        dpadVector.record(totalTime);
        int p = 10;
        for(p;p<150;p=p+10){
            timeUpDown = sizeUpDown/(p*1000);
            roadTime = hophop*timeUpDown;
            totalTime = 0.032 + nodeToSat + islTime + roadTime; //0.004 or 0.024
            EV<<"FOR A DATARATE OF "<<p<<" kbps THE DPAD is : "<<totalTime<<endl;
        }


        if(t1 < 1 || t1 > 4){     //1.712128
            EV<<"Can't send it"<<endl;
        }
        else if (t1 >= 1){
            //EV<<"NOOOOOOOOOOOOOOOOOOO "<<yep <<endl;
            EV<<"SENDING FROM NODE NUMBER "<<(numberOfTheNode-1)<<endl;
            EV<<"TO THE SAT NUMBER "<<numberOfTheSat<<endl;
            EV<<"THE DISTANCE BETWEEN THEM IS "<<dist<<endl;

        }
        //if(frame->getTmpPath1()==1){
          //  do
        //}
        //ACKNOWLEDGMENT MESSAGE SENDING
                    auto mgmtPacket = makeShared<LoRaAppPacket>();
                    mgmtPacket->setMsgType(ACK);
                    auto frameToSend = makeShared<LoRaMacFrame>();
                    frameToSend->setChunkLength(B(par("headerLength").intValue()));

                      //  LoRaMacFrame *frameToSend = new LoRaMacFrame("ADRPacket");

                        //frameToSend->encapsulate(mgmtPacket);
                    frameToSend->setReceiverAddress(frame->getTransmitterAddress());
                        //FIXME: What value to set for LoRa TP
                        //frameToSend->setLoRaTP(pkt->getLoRaTP());
                        //frameToSend->setLoRaTP(frame->getLoRaTP());!!!!
                    frameToSend->setLoRaTP(math::dBmW2mW(14));
                    frameToSend->setLoRaCF(frame->getLoRaCF());
                    frameToSend->setLoRaSF(frame->getLoRaSF());
                    frameToSend->setLoRaBW(frame->getLoRaBW());
                    frameToSend->setNumHop(frame->getNumHop());
                    frameToSend->setTmpPath6(frame->getTmpPath6());
                    frameToSend->setTmpPath5(frame->getTmpPath5());
                    frameToSend->setTmpPath4(frame->getTmpPath4());
                    frameToSend->setTmpPath3(frame->getTmpPath3());
                    frameToSend->setTmpPath2(frame->getTmpPath2());
                    frameToSend->setTmpPath1(frame->getTmpPath1());

                    auto pktAux = new Packet("HI I AM AN ACK MESSAGE !");
                    mgmtPacket->setChunkLength(B(par("headerLength").intValue()));

                    pktAux->insertAtFront(mgmtPacket);
                    pktAux->insertAtFront(frameToSend);
                    socket.sendTo(pktAux, pickedGateway, destPort);
                }
    if(evaluateADRinServer)
    {
        evaluateADR(pkt, pickedGateway, SNIRinGW, RSSIinGW);
    }
    delete receivedPackets[packetNumber].rcvdPacket;
    delete selfMsg;
    receivedPackets.erase(receivedPackets.begin()+packetNumber);
}

double NetworkServerApp::satToNodeDelayTime(int satNumber, int nodeNumber)
{
    string pathToFiles = "/home/mehdy/LEO_";
    string endOfPathToFiles = "-LLA-Pos.csv";
    string nodesFilePath = "/home/mehdy/SITES-LLA-Pos.csv";
    double lat_1;
    double lon_1;
    double alt_1;

    double lat_2;
    double lon_2;
    double alt_2;
    const double r = 6371; // Radius of Earth in Kilometres

    ifstream me(pathToFiles+to_string(satNumber)+endOfPathToFiles);
    if(!me.is_open()) std::cout <<"ERROR: File Open" << '\n';

    string TIME;
    string LAT_1;
    string LON_1;
    string ALT_1;

    int i=1;

    while(i<=k){

        getline(me,TIME,',');
        getline(me,LAT_1,',');
        getline(me,LON_1,',');
        getline(me,ALT_1,'\n');


        i++;
    }
    me.close();
    lat_1=strtod(LAT_1.c_str(),NULL);
    lon_1=strtod(LON_1.c_str(),NULL);
    alt_1=strtod(ALT_1.c_str(),NULL);

    EV<<"LATITUDE OF THE SAT "<<lat_1<<endl;
    EV<<"LONGITUDE OF THE SAT "<<lon_1<<endl;
    EV<<"ALTITUDE OF THE SAT "<<alt_1<<endl;

    double x_1 = (r + alt_1) * sin(lon_1) * cos(lat_1);
    double y_1 = (r + alt_1) * sin(lon_1) * sin(lat_1);
    double z_1 = (r + alt_1) * cos(lon_1);



    ifstream node(nodesFilePath);
    if(!node.is_open()) std::cout <<"ERROR: File Open" << '\n';

    string TIME2;
    string LAT_2;
    string LON_2;
    string ALT_2;

    i=0;
    while(i<=(nodeNumber)){

            getline(node,TIME2,',');
            getline(node,LAT_2,',');
            getline(node,LON_2,',');
            getline(node,ALT_2,'\n');


            i++;
    }
    node.close();

    lat_2=atof(LAT_2.c_str());
    lon_2=atof(LON_2.c_str());
    alt_2=atof(ALT_2.c_str());

    EV<<"LATITUDE OF THE NODE "<<lat_2<<endl;
    EV<<"LONGITUDE OF THE NODE "<<lon_2<<endl;
    EV<<"ALTITUDE OF THE NODE "<<alt_2<<endl;

    double x_2 = (r + alt_2) * sin(lon_2) * cos(lat_2);
    double y_2 = (r + alt_2) * sin(lon_2) * sin(lat_2);
    double z_2 = (r + alt_2) * cos(lon_2);

    //distance is calculated using spherical law of cosine with an extra step for the altitude
    double phi1 = lat_1 * (PI/180);
    double phi2 = lat_2 * (PI/180);
    double deltaPhi = (phi2-phi1) * (PI/180);
    double deltaLambda = (lon_2-lon_1) * (PI/180);
    double a = (sin((deltaPhi)/2)*sin((deltaPhi)/2))+(cos(phi1)*cos(phi2)*(sin((deltaLambda)/2)*sin((deltaLambda)/2)));
    double c = 2* (atan2((sqrt(a)),(sqrt(1-a))));
    double d = r*c;
    //double satToNodeDist = sqrt((d*d)+((alt_2-alt_1)*(alt_2-alt_1)));
    double dis1 = acos((sin(phi1)*sin(phi2))+(cos(phi1)*cos(phi2)*cos(deltaLambda)))*r;
    //double upDist = d9;
    double satToNodeDist = sqrt((dis1*dis1)+((alt_2-alt_1)*(alt_2-alt_1)));

    EV<<"THE DISTANCE BETWEEN THE NODE AND THE SAT IS : "<<satToNodeDist<<endl;
    return satToNodeDist;
}
void NetworkServerApp::calculateDelayTime()
{
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

    const double r = 6371; // Radius of Earth in Kilometres
    int m=0;

    for(int y=0;y<16;y++){

    ifstream me(pathToFiles+to_string(y)+endOfPathToFiles);
    if(!me.is_open()) std::cout <<"ERROR: File Open" << '\n';

    string TIME;
    string LAT_1;
    string LON_1;
    string ALT_1;

    int i=1;

    while(i<=k){

        getline(me,TIME,',');
        getline(me,LAT_1,',');
        getline(me,LON_1,',');
        getline(me,ALT_1,'\n');


        i++;
    }
    me.close();

    lat_1=strtod(LAT_1.c_str(),NULL);
    lon_1=strtod(LON_1.c_str(),NULL);
    alt_1=strtod(ALT_1.c_str(),NULL);




    double x_1 = (r + alt_1) * sin(lon_1) * cos(lat_1);
    double y_1 = (r + alt_1) * sin(lon_1) * sin(lat_1);
    double z_1 = (r + alt_1) * cos(lon_1);
    int tmp1=y;

    if(tmp1!=3){
        int tmp2=y;

        if(tmp2!=7){
            int tmp3=y;
            if(tmp3!=11){
                int tmp4=y;
                if(tmp4!=15){

    ifstream upNeighbor(pathToFiles+to_string((y+1))+endOfPathToFiles);
    if(!upNeighbor.is_open()) std::cout <<"ERROR: File Open" << '\n';

    string TIME2;
    string LAT_2;
    string LON_2;
    string ALT_2;

    i=1;

    while(i<=k){

        getline(upNeighbor,TIME2,',');
        getline(upNeighbor,LAT_2,',');
        getline(upNeighbor,LON_2,',');
        getline(upNeighbor,ALT_2,'\n');


        i++;
    }

    upNeighbor.close();
    lat_2=atof(LAT_2.c_str());
    lon_2=atof(LON_2.c_str());
    alt_2=atof(ALT_2.c_str());


    double x_2 = (r + alt_2) * sin(lon_2) * cos(lat_2);
    double y_2 = (r + alt_2) * sin(lon_2) * sin(lat_2);
    double z_2 = (r + alt_2) * cos(lon_2);


    double phi1 = lat_1 * (PI/180);
    double phi2 = lat_2 * (PI/180);
    double deltaPhi = (phi2-phi1) * (PI/180);
    double deltaLambda = (lon_2-lon_1) * (PI/180);
    double a = (sin((deltaPhi)/2)*sin((deltaPhi)/2))+(cos(phi1)*cos(phi2)*(sin((deltaLambda)/2)*sin((deltaLambda)/2)));
    double c = 2* (atan2((sqrt(a)),(sqrt(1-a))));
    double d = r*c;
    double d9 = acos((sin(phi1)*sin(phi2))+(cos(phi1)*cos(phi2)*cos(deltaLambda)))*r;
    //double upDist = d9;
    double upDist = sqrt((d9*d9)+((alt_2-alt_1)*(alt_2-alt_1)));
    //double upDist = sqrt((x_2 - x_1) * (x_2 - x_1) + (y_2 - y_1) * (y_2 - y_1) + (z_2 - z_1) * (z_2 - z_1));
    EV<<"THE DISTANCE BETWEEN SATELLITE "<<y<<" AND IT s UPPER NEIGHBOR IS "<<upDist<<" KKNOWING POSITION OF NODE : "<<lat_1<<" "<<lon_1<<" AND POSITION OF SAT "<<lat_2<<" "<<lon_2<<endl;
    pathToRightGW[m]=(upDist / 200000);
    m++;

    }
            }}}
    int tmp6=y;
    if(tmp6<12){
    ifstream rightNeighbor(pathToFiles+to_string((y+4))+endOfPathToFiles);
        if(!rightNeighbor.is_open()) std::cout <<"ERROR: File Open" << '\n';

    string TIME3;
    string LAT_3;
    string LON_3;
    string ALT_3;

    i=1;

    while(i<=k){

        getline(rightNeighbor,TIME3,',');
        getline(rightNeighbor,LAT_3,',');
        getline(rightNeighbor,LON_3,',');
        getline(rightNeighbor,ALT_3,'\n');


        i++;
    }

    rightNeighbor.close();

    lat_3=atof(LAT_3.c_str());
    lon_3=atof(LON_3.c_str());
    alt_3=atof(ALT_3.c_str());


    double x_3 = (r + alt_3) * sin(lon_3) * cos(lat_3);
    double y_3 = (r + alt_3) * sin(lon_3) * sin(lat_3);
    double z_3 = (r + alt_3) * cos(lon_3);

    double phi1 = lat_1 * (PI/180);
    double phi3 = lat_3 * (PI/180);
    double deltaPhi2 = (phi3-phi1) * (PI/180);
    double deltaLambda2 = (lon_3-lon_1) * (PI/180);
    double a2 = (sin((deltaPhi2)/2)*sin((deltaPhi2)/2))+(cos(phi1)*cos(phi3)*(sin((deltaLambda2)/2)*sin((deltaLambda2)/2)));
    double c2 = 2* (atan2((sqrt(a2)),(sqrt(1-a2))));
    double d2 = r*c2;
    double d8 = acos((sin(phi1)*sin(phi3))+(cos(phi1)*cos(phi3)*cos(deltaLambda2)))*r;
    //double rightDist=d8;
    double rightDist = sqrt((d8*d8)+((alt_3-alt_1)*(alt_3-alt_1)));
    //double rightDist = sqrt((x_3 - x_1) * (x_3 - x_1) + (y_3 - y_1) * (y_3 - y_1) + (z_3 - z_1) * (z_3 - z_1));
    pathToRightGW[m]=(rightDist/200000);
    m++;
    }
    }
    m=0;
       k++;

}
void NetworkServerApp::evaluateADR(Packet* pkt, L3Address pickedGateway, double SNIRinGW, double RSSIinGW)
{
    bool sendADR = false;
    bool sendADRAckRep = false;
    double SNRm; //needed for ADR
    int nodeIndex;

    pkt->trimFront();
    auto frame = pkt->removeAtFront<LoRaMacFrame>();

    const auto & rcvAppPacket = pkt->peekAtFront<LoRaAppPacket>();

    if(rcvAppPacket->getOptions().getADRACKReq())
    {
        sendADRAckRep = true;
    }

    for(uint i=0;i<knownNodes.size();i++)
    {
        if(knownNodes[i].srcAddr == frame->getTransmitterAddress())
        {
            knownNodes[i].adrListSNIR.push_back(SNIRinGW);
            knownNodes[i].historyAllSNIR->record(SNIRinGW);
            knownNodes[i].historyAllRSSI->record(RSSIinGW);
            knownNodes[i].receivedSeqNumber->record(frame->getSequenceNumber());
            if(knownNodes[i].adrListSNIR.size() == 20) knownNodes[i].adrListSNIR.pop_front();
            knownNodes[i].framesFromLastADRCommand++;

            if(knownNodes[i].framesFromLastADRCommand == 20 || sendADRAckRep == true)
            {
                nodeIndex = i;
                knownNodes[i].framesFromLastADRCommand = 0;
                sendADR = true;
                if(adrMethod == "max")
                {
                    SNRm = *max_element(knownNodes[i].adrListSNIR.begin(), knownNodes[i].adrListSNIR.end());
                }
                if(adrMethod == "avg")
                {
                    double totalSNR = 0;
                    int numberOfFields = 0;
                    for (std::list<double>::iterator it=knownNodes[i].adrListSNIR.begin(); it != knownNodes[i].adrListSNIR.end(); ++it)
                    {
                        totalSNR+=*it;
                        numberOfFields++;
                    }
                    SNRm = totalSNR/numberOfFields;
                }

            }

        }
    }

    if(sendADR || sendADRAckRep)
    {
        auto mgmtPacket = makeShared<LoRaAppPacket>();
        mgmtPacket->setMsgType(TXCONFIG);

        if(sendADR)
        {
            double SNRmargin;
            double requiredSNR;
            if(frame->getLoRaSF() == 7) requiredSNR = -7.5;
            if(frame->getLoRaSF() == 8) requiredSNR = -10;
            if(frame->getLoRaSF() == 9) requiredSNR = -12.5;
            if(frame->getLoRaSF() == 10) requiredSNR = -15;
            if(frame->getLoRaSF() == 11) requiredSNR = -17.5;
            if(frame->getLoRaSF() == 12) requiredSNR = -20;

            SNRmargin = SNRm - requiredSNR - adrDeviceMargin;
            knownNodes[nodeIndex].calculatedSNRmargin->record(SNRmargin);
            int Nstep = round(SNRmargin/3);
            LoRaOptions newOptions;

            // Increase the data rate with each step
            int calculatedSF = frame->getLoRaSF();
            while(Nstep > 0 && calculatedSF > 7)
            {
                calculatedSF--;
                Nstep--;
            }

            // Decrease the Tx power by 3 for each step, until min reached
            double calculatedPowerdBm = math::mW2dBmW(frame->getLoRaTP()) + 30;
            while(Nstep > 0 && calculatedPowerdBm > 2)
            {
                calculatedPowerdBm-=3;
                Nstep--;
            }
            if(calculatedPowerdBm < 2) calculatedPowerdBm = 2;

            // Increase the Tx power by 3 for each step, until max reached
            while(Nstep < 0 && calculatedPowerdBm < 14)
            {
                calculatedPowerdBm+=3;
                Nstep++;
            }
            if(calculatedPowerdBm > 14) calculatedPowerdBm = 14;

            newOptions.setLoRaSF(calculatedSF);
            newOptions.setLoRaTP(calculatedPowerdBm);
            EV << calculatedSF << endl;
            EV << calculatedPowerdBm << endl;
            mgmtPacket->setOptions(newOptions);
        }

        if(simTime() >= getSimulation()->getWarmupPeriod() && sendADR == true)
        {
            knownNodes[nodeIndex].numberOfSentADRPackets++;
        }

        auto frameToSend = makeShared<LoRaMacFrame>();
        frameToSend->setChunkLength(B(par("headerLength").intValue()));

      //  LoRaMacFrame *frameToSend = new LoRaMacFrame("ADRPacket");

        //frameToSend->encapsulate(mgmtPacket);
        frameToSend->setReceiverAddress(frame->getTransmitterAddress());
        //FIXME: What value to set for LoRa TP
        //frameToSend->setLoRaTP(pkt->getLoRaTP());
        //frameToSend->setLoRaTP(frame->getLoRaTP());!!!!
        frameToSend->setLoRaTP(math::dBmW2mW(14));
        frameToSend->setLoRaCF(frame->getLoRaCF());
        frameToSend->setLoRaSF(frame->getLoRaSF());
        frameToSend->setLoRaBW(frame->getLoRaBW());

        auto pktAux = new Packet("ADRPacket");
        mgmtPacket->setChunkLength(B(par("headerLength").intValue()));

        pktAux->insertAtFront(mgmtPacket);
        pktAux->insertAtFront(frameToSend);
        socket.sendTo(pktAux, pickedGateway, destPort);

    }
/*    if(WorkWithAck){
        auto mgmtPacket = makeShared<LoRaAppPacket>();
        mgmtPacket->setMsgType(ACK);
        auto frameToSend = makeShared<LoRaMacFrame>();
        frameToSend->setChunkLength(B(par("headerLength").intValue()));

          //  LoRaMacFrame *frameToSend = new LoRaMacFrame("ADRPacket");

            //frameToSend->encapsulate(mgmtPacket);
        frameToSend->setReceiverAddress(frame->getTransmitterAddress());
            //FIXME: What value to set for LoRa TP
            //frameToSend->setLoRaTP(pkt->getLoRaTP());
            //frameToSend->setLoRaTP(frame->getLoRaTP());!!!!
        frameToSend->setLoRaTP(math::dBmW2mW(14));
        frameToSend->setLoRaCF(frame->getLoRaCF());
        frameToSend->setLoRaSF(frame->getLoRaSF());
        frameToSend->setLoRaBW(frame->getLoRaBW());

        auto pktAux = new Packet("HI I AM AN ACK MESSAGE !");
        mgmtPacket->setChunkLength(B(par("headerLength").intValue()));

        pktAux->insertAtFront(mgmtPacket);
        pktAux->insertAtFront(frameToSend);
        socket.sendTo(pktAux, pickedGateway, destPort);
    }
 */   //delete pkt;
}

void NetworkServerApp::receiveSignal(cComponent *source, simsignal_t signalID, intval_t value, cObject *details)
{
    if (simTime() >= getSimulation()->getWarmupPeriod())
    {
        counterOfSentPacketsFromNodes++;
        counterOfSentPacketsFromNodesPerSF[value-7]++;
    }
}

} //namespace inet
