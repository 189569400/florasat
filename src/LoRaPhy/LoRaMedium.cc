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
#include "LoRaMedium.h"
#include "inet/common/INETUtils.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/Simsignals.h"
#include "inet/common/ProtocolTag_m.h"
//#include "inet/linklayer/contract/IMACFrame.h"
#include "inet/linklayer/common/MacAddressTag_m.h"
#include "inet/networklayer/common/NetworkInterface.h"
#include "inet/networklayer/contract/IInterfaceTable.h"
#include "inet/physicallayer/wireless/common/contract/packetlevel/IInterference.h"
#include "inet/physicallayer/wireless/common/radio/packetlevel/Radio.h"
#include "inet/physicallayer/wireless/common/medium/RadioMedium.h"
#include "inet/physicallayer/wireless/common/contract/packetlevel/SignalTag_m.h"
#include "inet/physicallayer/wireless/common/contract/packetlevel/IErrorModel.h"

#include "libnorad/cEcef.h"
#include "LoRa/LoRaMacFrame_m.h"
#include "LoRaTransmission.h"

namespace flora {

Define_Module(LoRaMedium);

LoRaMedium::LoRaMedium() : RadioMedium()
{
}

LoRaMedium::~LoRaMedium()
{
}

void LoRaMedium::initialize(int stage)
{
    RadioMedium::initialize(stage);
    mapX = std::atoi(getParentModule()->getDisplayString().getTagArg("bgb", 0));
    mapY = std::atoi(getParentModule()->getDisplayString().getTagArg("bgb", 1));
}

bool LoRaMedium::matchesMacAddressFilter(const IRadio *radio, const Packet *packet) const
{
    const auto &chunk = packet->peekAtFront<Chunk>();
    const auto & loraHeader = dynamicPtrCast<const LoRaMacFrame>(chunk);
    if (loraHeader == nullptr)
        return false;
    MacAddress address = MacAddress(loraHeader->getReceiverAddress().getInt());
    if (address.isBroadcast() || address.isMulticast())
        return true;

    cModule *host = getContainingNode(check_and_cast<const cModule *>(radio));
    IInterfaceTable *interfaceTable = check_and_cast<IInterfaceTable *>(host->getSubmodule("interfaceTable"));
    for (int i = 0; i < interfaceTable->getNumInterfaces(); i++) {
        auto interface = interfaceTable->getInterface(i);
        if (interface && interface->getMacAddress() == address)
            return true;
    }
    return false;
}

bool LoRaMedium::isInCommunicationRange(const ITransmission *transmission, const Coord& startPosition, const Coord& endPosition) const
{
    m maxCommunicationRange = mediumLimitCache->getMaxCommunicationRange();

    const LoRaTransmission *loRaTransmission = check_and_cast<const LoRaTransmission *>(transmission);
    double transmitterLat = loRaTransmission->getStartLongLatPosition().m_Lat;
    double transmitterLon = loRaTransmission->getStartLongLatPosition().m_Lon;
    double transmitterAlt = loRaTransmission->getStartLongLatPosition().m_Alt;

    double receiverX = startPosition.getX();
    double receiverY = startPosition.getY();
    double receiverZ = startPosition.getZ();

    double receiverLon = (360 * receiverX / mapX) - 180;
    double receiverLat = 90 - (180 * receiverY / mapY);
    double receiverAlt = receiverZ;

    double range = 2830000; // 2830km for 600km altitude

    cEcef *transmitterEcef = new cEcef(transmitterLat, transmitterLon, transmitterAlt);
    cEcef *receiverEcef = new cEcef(receiverLat, receiverLon, receiverAlt);
    double distance = transmitterEcef->getDistance(*receiverEcef);

    delete transmitterEcef;
    delete receiverEcef;

    // ideally check also end position, but florasat does not support movement during
    // transmission, propagation and/or reception

    return distance < range;
    //return std::isnan(maxCommunicationRange.get()) || (distance < range);
}

const IReceptionResult *LoRaMedium::getReceptionResult(const IRadio *radio, const IListening *listening, const ITransmission *transmission) const
{
    cacheResultGetCount++;
    const IReceptionResult *result = communicationCache->getCachedReceptionResult(radio, transmission);
    if (result)
        cacheResultHitCount++;
    else {
        result = computeReceptionResult(radio, listening, transmission);

        auto pkt = const_cast<Packet *>(result->getPacket());
        if (!pkt->findTag<SnirInd>()) {
            const ISnir *snir = getSNIR(radio, transmission);
            auto snirInd = pkt->addTagIfAbsent<SnirInd>();
            snirInd->setMinimumSnir(snir->getMin());
            snirInd->setMaximumSnir(snir->getMax());
        }
        if (!pkt->findTag<ErrorRateInd>()) {
            auto errorModel = dynamic_cast<IErrorModel *>(getSubmodule("errorModel"));
            const ISnir *snir = getSNIR(radio, transmission);
            auto errorRateInd = pkt->addTagIfAbsent<ErrorRateInd>(); // TODO: should be done  setPacketErrorRate(packetModel->getPER());
            errorRateInd->setPacketErrorRate(errorModel ? errorModel->computePacketErrorRate(snir, IRadioSignal::SIGNAL_PART_WHOLE) : 0.0);
            errorRateInd->setBitErrorRate(errorModel ? errorModel->computeBitErrorRate(snir, IRadioSignal::SIGNAL_PART_WHOLE) : 0.0);
            errorRateInd->setSymbolErrorRate(errorModel ? errorModel->computeSymbolErrorRate(snir, IRadioSignal::SIGNAL_PART_WHOLE) : 0.0);
        }

        communicationCache->setCachedReceptionResult(radio, transmission, result);
        EV_DEBUG << "Receiving " << transmission << " from medium by " << radio << " arrives as " << result->getReception() << " and results in " << result << endl;
    }
    return result;
}

}
