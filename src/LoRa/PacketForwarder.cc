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

#include "PacketForwarder.h"
#include "ISLPacketForwarder.h"
//#include "inet/networklayer/ipv4/IPv4Datagram.h"
//#include "inet/networklayer/contract/ipv4/IPv4ControlInfo.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/common/ModuleAccess.h"
#include "inet/applications/base/ApplicationPacket_m.h"
#include "../LoRaPhy/LoRaRadioControlInfo_m.h"
#include "inet/physicallayer/wireless/common/contract/packetlevel/SignalTag_m.h"


namespace flora {

Define_Module(PacketForwarder);


void PacketForwarder::initialize(int stage)
{
    if (stage == 0) {
        LoRa_GWPacketReceived = registerSignal("LoRa_GWPacketReceived");
        localPort = par("localPort");
        destPort = par("destPort");
    } else if (stage == INITSTAGE_APPLICATION_LAYER) {
        startUDP();
        getSimulation()->getSystemModule()->subscribe("LoRa_AppPacketSent", this);
    }
}


void PacketForwarder::startUDP()
{
    EV << "Wywalamy sie tutaj" << endl;
    socket.setOutputGate(gate("socketOut"));
    const char *localAddress = par("localAddress");
    socket.bind(*localAddress ? L3AddressResolver().resolve(localAddress) : L3Address(), localPort);
    EV << "Dojechalismy za pierwszy resolv" << endl;
    // TODO: is this required?
    //setSocketOptions();

    const char *destAddrs = par("destAddresses");
    cStringTokenizer tokenizer(destAddrs);
    const char *token;

    // Create UDP sockets to multiple destination addresses (network servers)
    while ((token = tokenizer.nextToken()) != nullptr) {
        EV << "Wchodze w petle" << endl;
        EV << token << endl;
        L3Address result;
        L3AddressResolver().tryResolve(token, result);
        EV << "Wychodze z petli" << endl;
        if (result.isUnspecified())
            EV_ERROR << "cannot resolve destination address: " << token << endl;
        else
            EV << "Got destination address: " << token << endl;
        destAddresses.push_back(result);
    }
    EV << "Dojechalismy do konca" << endl;
}


void PacketForwarder::handleMessage(cMessage *msg)
{
    EV << msg->getArrivalGate() << endl;
    //auto meh = getContainingNode(this);
    //auto meh2 = check_and_cast_nullable<ISLPacketForwarder *>(meh->getSubmodule("islPacketForwarder"));
    //int myActualID = meh2->par("satelliteID");
    if (msg->arrivedOn("lowerLayerIn")) {
        EV << "Received LoRaMAC frame" << endl;

        auto pkt = check_and_cast<Packet*>(msg);
        //////auto frame = pkt->removeAtFront<LoRaMacFrame>();
        pkt->trimFront();
        auto frame = pkt->removeAtFront<LoRaMacFrame>();

      //  auto myRealGW = getContainingNode(this);
       // auto theTruth = myRealGW->getSubmodule("IslPacketForwarder");

        //frame->setSatNumber(theTruth->par("satelliteID"));
        frame->setPktType(UPLINK);
        ////////const auto &frame = pkt->peekAtFront<LoRaMacFrame>();
/*
        auto frameToSend = makeShared<LoRaMacFrame>();
        frameToSend->setChunkLength(B(8));
        //frame2->setLoRaSF(11);
        frameToSend->setPktType(UPLINK);
        frameToSend->setReceiverAddress(frame->getReceiverAddress());
        //frameToSend->setiverAddress(frame->getReceiverAddress());
        //FIXME: What value to set for LoRa TP
        //frameToSend->setLoRaTP(pkt->getLoRaTP());
        frameToSend->setLoRaTP(frame->getLoRaTP());

        //frameToSend->setLoRaTP(math::dBmW2mW(14));
        frameToSend->setLoRaCF(frame->getLoRaCF());
        frameToSend->setLoRaSF(frame->getLoRaSF());
        frameToSend->setLoRaBW(frame->getLoRaBW());
        frameToSend->setTransmitterAddress(frame->getTransmitterAddress());
        //frameToSend->setChunkLength(B(par("headerLength").intValue()));
*/
        //////pkt->trimFront();
        /////pkt->removeAtFront<LoRaMacFrame>();
        //auto frame2 = pkt->removeAtFront<LoRaMacFrame>();

        pkt->insertAtFront(frame);




      //  LoRaMacFrame *frameToSend = new LoRaMacFrame("ADRPacket");

        //frameToSend->encapsulate(mgmtPacket);
        //frameToSend->setReceiverAddress(frame->getReceiverAddress());
        //frameToSend->setiverAddress(frame->getReceiverAddress());
        //FIXME: What value to set for LoRa TP
        //frameToSend->setLoRaTP(pkt->getLoRaTP());
        //frameToSend->setLoRaTP(frame->getLoRaTP());!!!!

        //frameToSend->setLoRaTP(math::dBmW2mW(14));
        //frameToSend->setLoRaCF(frame->getLoRaCF());
        //frameToSend->setLoRaSF(frame->getLoRaSF());
        //frameToSend->setLoRaBW(frame->getLoRaBW());

        //const auto & rcvAppPacket = pkt->peekAtFront<LoRaAppPacket>();

        //frame->setPktType(UPLINK);
        //auto pktBack = new Packet("LoraPacket");
        //auto frameToSend = makeShared<LoRaMacFrame>();
        //frameToSend->setChunkLength(B(par("headerLength").intValue()));

        //frameToSend->setReceiverAddress(frame->getTransmitterAddress());
        //pktBack->insertAtFront(frameToSend);
        //sendDown(pktBack);

        send(pkt,"loRaPart$o");
        //if(frame->getReceiverAddress() == MacAddress::BROADCAST_ADDRESS)
            //processLoraMACPacket(pkt);


        //send(msg, "upperLayerOut");
        //sendPacket();
    } else if (msg->arrivedOn("socketIn")) {
        // FIXME : debug for now to see if LoRaMAC frame received correctly from network server
        EV << "Received UDP packet" << endl;

        // FIXME : Catch Indication error for unroutable packets
        EV_ERROR << "Catch Indication error...\n";
        if (dynamic_cast<inet::Indication *>(msg)) {
            EV_ERROR << "Indication error received -- discarding\n";
            delete msg;
            return;
        }

        auto pkt = check_and_cast<Packet*>(msg);

        //const auto &frame = pkt->peekAtFront<LoRaMacFrame>();

        pkt->trimFront();
        auto frame = pkt->removeAtFront<LoRaMacFrame>();

        frame->setPktType(DOWNLINK);
/*
        auto frameToSend = makeShared<LoRaMacFrame>();
        frameToSend->setChunkLength(B(8));
        //frame2->setLoRaSF(11);
        frameToSend->setPktType(DOWNLINK);
        frameToSend->setReceiverAddress(frame->getReceiverAddress());
        //frameToSend->setiverAddress(frame->getReceiverAddress());
        //FIXME: What value to set for LoRa TP
        //frameToSend->setLoRaTP(pkt->getLoRaTP());
        frameToSend->setLoRaTP(frame->getLoRaTP());

        //frameToSend->setLoRaTP(math::dBmW2mW(14));
        frameToSend->setLoRaCF(frame->getLoRaCF());
        frameToSend->setLoRaSF(frame->getLoRaSF());
        frameToSend->setLoRaBW(frame->getLoRaBW());
        frameToSend->setTransmitterAddress(frame->getTransmitterAddress());
        frameToSend->setNumHop(frame->getNumHop());
        frameToSend->setTmpPath6(frame->getTmpPath6());
        frameToSend->setTmpPath5(frame->getTmpPath5());
        frameToSend->setTmpPath4(frame->getTmpPath4());
        frameToSend->setTmpPath3(frame->getTmpPath3());
        frameToSend->setTmpPath2(frame->getTmpPath2());
        frameToSend->setTmpPath1(frame->getTmpPath1());
        frameToSend->setReceiverAddress(frame->getReceiverAddress());
*/
        //frameToSend->setChunkLength(B(par("headerLength").intValue()));

        /////pkt->trimFront();
        /////pkt->removeAtFront<LoRaMacFrame>();
        //auto frame2 = pkt->removeAtFront<LoRaMacFrame>();

        pkt->insertAtFront(frame);
        send(pkt, "loRaPart$o");

        //if (frame == nullptr)
          //  throw cRuntimeError("Packet type error");


        //EV << frame->getLoRaTP() << endl;
        //delete frame;

       /* auto loraTag = pkt->addTagIfAbsent<LoRaTag>();
        pkt->setBandwidth(loRaBW);
        pkt->setCarrierFrequency(loRaCF);
        pkt->setSpreadFactor(loRaSF);
        pkt->setCodeRendundance(loRaCR);
        pkt->setPower(W(loRaTP));*/

        //send(pkt, "lowerLayerOut");

        //
    }
    else if (msg->arrivedOn("loRaPart$i")){
        auto pkt = check_and_cast<Packet*>(msg);
        const auto &frame = pkt->peekAtFront<LoRaMacFrame>();
        if(frame->getPktType() == UPLINK){
        if(frame->getReceiverAddress() == MacAddress::BROADCAST_ADDRESS)
        processLoraMACPacket(pkt);
        }
        else if(frame->getPktType()==DOWNLINK){
            if (frame == nullptr)
                throw cRuntimeError("Packet type error");
            //EV << frame->getLoRaTP() << endl;
            //delete frame;

           /* auto loraTag = pkt->addTagIfAbsent<LoRaTag>();
            pkt->setBandwidth(loRaBW);
            pkt->setCarrierFrequency(loRaCF);
            pkt->setSpreadFactor(loRaSF);
            pkt->setCodeRendundance(loRaCR);
            pkt->setPower(W(loRaTP));*/

            send(pkt, "lowerLayerOut");
        }
    }
}

void PacketForwarder::processLoraMACPacket(Packet *pk)
{
    // FIXME: Change based on new implementation of MAC frame.
    emit(LoRa_GWPacketReceived, 42);
    if (simTime() >= getSimulation()->getWarmupPeriod())
        counterOfReceivedPackets++;
    pk->trimFront();
    auto frame = pk->removeAtFront<LoRaMacFrame>();

    auto snirInd = pk->getTag<SnirInd>();

    auto signalPowerInd = pk->getTag<SignalPowerInd>();

    W w_rssi = signalPowerInd->getPower();
    double rssi = w_rssi.get()*1000;
    frame->setRSSI(math::mW2dBmW(rssi));
    frame->setSNIR(snirInd->getMinimumSnir());
    pk->insertAtFront(frame);

    //bool exist = false;
    EV << frame->getTransmitterAddress() << endl;
    //for (std::vector<nodeEntry>::iterator it = knownNodes.begin() ; it != knownNodes.end(); ++it)

    // FIXME : Identify network server message is destined for.
    L3Address destAddr = destAddresses[0];
    if (pk->getControlInfo())
       delete pk->removeControlInfo();

    socket.sendTo(pk, destAddr, destPort);
}

void PacketForwarder::sendPacket()
{
//    LoRaAppPacket *mgmtCommand = new LoRaAppPacket("mgmtCommand");
//    mgmtCommand->setMsgType(TXCONFIG);
//    LoRaOptions newOptions;
//    newOptions.setLoRaTP(uniform(0.1, 1));
//    mgmtCommand->setOptions(newOptions);

//    LoRaMacFrame *response = new LoRaMacFrame("mgmtCommand");
//    response->encapsulate(mgmtCommand);
//    response->setLoRaTP(pk->getLoRaTP());
//    response->setLoRaCF(pk->getLoRaCF());
//    response->setLoRaSF(pk->getLoRaSF());
//    response->setLoRaBW(pk->getLoRaBW());
//    response->setReceiverAddress(pk->getTransmitterAddress());
//    send(response, "lowerLayerOut");

}

void PacketForwarder::receiveSignal(cComponent *source, simsignal_t signalID, intval_t value, cObject *details)
{
    if (simTime() >= getSimulation()->getWarmupPeriod())
        counterOfSentPacketsFromNodes++;
}

void PacketForwarder::finish()
{
    recordScalar("LoRa_GW_DER", double(counterOfReceivedPackets)/counterOfSentPacketsFromNodes);
}



} //namespace inet
