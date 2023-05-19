/*
 * GroundstationInfo.cc
 *
 * Created on: Feb 06, 2023
 *     Author: Robin Ohs
 */

#include "GroundStationRouting.h"

namespace flora {
namespace ground {

Define_Module(GroundStationRouting);

void GroundStationRouting::initialize(int stage) {
    if (stage == INITSTAGE_LOCAL) {
        groundStationId = getIndex();
        mobility = check_and_cast<GroundStationMobility*>(getSubmodule("gsMobility"));
        satellites = std::set<int>();
    }
}

cGate* GroundStationRouting::getInputGate(int index) {
    return gateHalf(Constants::GS_SATLINK_NAME, cGate::Type::INPUT, index);
}

cGate* GroundStationRouting::getOutputGate(int index) {
    return gateHalf(Constants::GS_SATLINK_NAME, cGate::Type::OUTPUT, index);
}

double GroundStationRouting::getLongitude() const {
    return mobility->getLUTPositionX();
}

double GroundStationRouting::getLatitude() const {
    return mobility->getLUTPositionY();
}

double GroundStationRouting::getAltitude() const {
    return 0;
}

const std::set<int>& GroundStationRouting::getSatellites() const {
    return satellites;
}

void GroundStationRouting::addSatellite(int satId) {
    ASSERT(!core::utils::set::contains(satellites, satId));
    satellites.emplace(satId);
}

void GroundStationRouting::removeSatellite(int satId) {
    ASSERT(core::utils::set::contains(satellites, satId));
    satellites.erase(satId);
}

bool GroundStationRouting::isConnectedToAnySat() {
    return !satellites.empty();
}

bool GroundStationRouting::isConnectedTo(int satId) {
    return core::utils::set::contains(satellites, satId);
}

std::string to_string(const GroundStationRouting& gs) {
    std::ostringstream ss;
    ss << gs;
    return ss.str();
}

}  // namespace ground
}  // namespace flora
