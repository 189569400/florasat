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

#ifndef LORA_LORAGWMAC_H_
#define LORA_LORAGWMAC_H_

#include "inet/common/INETDefs.h"
#include "inet/physicallayer/wireless/common/contract/packetlevel/IRadio.h"
#include "inet/linklayer/contract/IMacProtocol.h"
#include "inet/linklayer/base/MacProtocolBase.h"
#include "inet/linklayer/common/InterfaceTag_m.h"
#include "inet/linklayer/common/MacAddressTag_m.h"
#include "inet/common/ModuleAccess.h"
//#include "inet/transportlayer/contract/udp/UdpSocket.h"

#include "LoRaMacControlInfo_m.h"
#include "LoRaMacFrame_m.h"
//#include "LoRaApp/LoRaAppPacket_m.h"

#if INET_VERSION < 0x0403 || ( INET_VERSION == 0x0403 && INET_PATCH_LEVEL == 0x00 )
#  error At least INET 4.3.1 is required. Please update your INET dependency and fully rebuild the project.
#endif
namespace flora {

using namespace inet;
using namespace inet::physicallayer;

class LoRaGWMac: public MacProtocolBase {
public:

    bool waitingForDC;
    cMessage *dutyCycleTimer;
    cMessage *beaconPeriod;

    virtual void initialize(int stage) override;
    virtual void finish() override;

    virtual void configureNetworkInterface() override;
    long GW_forwardedDown;
    long GW_droppedDC;

    virtual void handleUpperMessage(cMessage *msg) override;
    virtual void handleLowerMessage(cMessage *msg) override;
    virtual void handleSelfMessage(cMessage *message) override;

    void sendPacketBack(Packet *receivedFrame);
    void sendBeacon();
    virtual MacAddress getAddress();

protected:

    int outGate = -1;
    MacAddress address;

    int lastSentMeasurement;
    int beaconTimer;
    int pingNumber;
    int beaconStart;

    int beaconSF;
    int beaconCR = -1;
    double beaconTP;
    double beaconCF;
    double beaconBW;

    IRadio *radio = nullptr;
    IRadio::TransmissionState transmissionState = IRadio::TRANSMISSION_STATE_UNDEFINED;

    virtual void receiveSignal(cComponent *source, simsignal_t signalID, intval_t value, cObject *details) override;
};

}

#endif /* LORA_LORAGWMAC_H_ */
