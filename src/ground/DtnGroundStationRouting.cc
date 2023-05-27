/*
 * GroundstationInfo.cc
 *
 * Created on: Feb 06, 2023
 *     Author: Robin Ohs
 */

#include "DtnGroundStationRouting.h"

namespace flora {
namespace ground {

Define_Module(DtnGroundStationRouting);

void DtnGroundStationRouting::initialize(int stage) {
    if (stage == INITSTAGE_LOCAL) {
        groundStationId = getIndex();
        mobility = check_and_cast<GroundStationMobility*>(getSubmodule("gsMobility"));
        satellites = std::set<int>();
    }
}

cGate* DtnGroundStationRouting::getInputGate(int index) {
    return gateHalf(Constants::GS_SATLINK_NAME, cGate::Type::INPUT, index);
}

cGate* DtnGroundStationRouting::getOutputGate(int index) {
    return gateHalf(Constants::GS_SATLINK_NAME, cGate::Type::OUTPUT, index);
}

double DtnGroundStationRouting::getLongitude() const {
    return mobility->getLUTPositionX();
}

double DtnGroundStationRouting::getLatitude() const {
    return mobility->getLUTPositionY();
}

double DtnGroundStationRouting::getAltitude() const {
    return 0;
}

const std::set<int>& DtnGroundStationRouting::getSatellites() const {
    return satellites;
}

void DtnGroundStationRouting::addSatellite(int satId) {
    ASSERT(!core::utils::set::contains(satellites, satId));
    satellites.emplace(satId);
}

void DtnGroundStationRouting::removeSatellite(int satId) {
    ASSERT(core::utils::set::contains(satellites, satId));
    satellites.erase(satId);
}

bool DtnGroundStationRouting::isConnectedToAnySat() {
    return !satellites.empty();
}

bool DtnGroundStationRouting::isConnectedTo(int satId) {
    return core::utils::set::contains(satellites, satId);
}

std::string to_string(const DtnGroundStationRouting& gs) {
    std::ostringstream ss;
    ss << gs;
    return ss.str();
}

}  // namespace ground
}  // namespace flora
