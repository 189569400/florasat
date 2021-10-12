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

#ifndef __LORANETWORK_ISLPACKETFORWARDER_H_
#define __LORANETWORK_ISLPACKETFORWARDER_H_

#include <omnetpp.h>
#include "inet/physicallayer/wireless/common/contract/packetlevel/RadioControlInfo_m.h"
#include <vector>
#include "inet/common/INETDefs.h"

#include "LoRaMacControlInfo_m.h"
#include "LoRaMacFrame_m.h"
#include "inet/applications/base/ApplicationBase.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"

namespace flora {

class ISLPacketForwarder : public cSimpleModule, public cListener
{
  protected:
    std::vector<L3Address> destAddresses;
    int localPort = -1, destPort = -1;
    int calculateDistance;
    int deviceID;
    // state
    UdpSocket socket;
    cMessage *selfMsg = nullptr;

  protected:
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;
    void startUDP();

    void setSocketOptions();
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
  public:
    int k = 1201;
    void distanceCalculation(int devId);
    cMessage *calculDistance;
    cMessage *sendRight;
      simsignal_t LoRa_GWPacketReceived;
      int counterOfSentPacketsFromNodes = 0;
      int counterOfReceivedPackets = 0;
};
} //namespace inet
#endif
