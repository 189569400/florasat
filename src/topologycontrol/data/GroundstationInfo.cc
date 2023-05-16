/*
 * GroundstationInfo.cc
 *
 * Created on: Feb 06, 2023
 *     Author: Robin Ohs
 */

#include "GroundstationInfo.h"

namespace flora {
namespace topologycontrol {

cGate* GroundstationInfo::getInputGate(int index) const {
    return groundStation->gateHalf(core::Constants::GS_SATLINK_NAME, cGate::Type::INPUT, index);
}

cGate* GroundstationInfo::getOutputGate(int index) const {
    return groundStation->gateHalf(core::Constants::GS_SATLINK_NAME, cGate::Type::OUTPUT, index);
}

double GroundstationInfo::getLongitude() const {
    return mobility->getLUTPositionX();
}

double GroundstationInfo::getLatitude() const {
    return mobility->getLUTPositionY();
}

double GroundstationInfo::getAltitude() const {
    return 0;
}

std::string to_string(const GroundstationInfo& gsInfo) {
    std::ostringstream ss;
    ss << gsInfo;
    return ss.str();
}

}  // namespace topologycontrol
}  // namespace flora
