/*
 * SatelliteInfo.cc
 *
 * Created on: Feb 10, 2023
 *     Author: Robin Ohs
 */

#include "SatelliteInfo.h"

namespace flora {
namespace topologycontrol {

cGate* SatelliteInfo::getInputGate(isldirection::Direction direction, int index) const {
    switch (direction) {
        case isldirection::Direction::ISL_LEFT:
            return satelliteModule->gateHalf(Constants::ISL_LEFT_NAME, cGate::Type::INPUT, -1);
        case isldirection::Direction::ISL_UP:
            return satelliteModule->gateHalf(Constants::ISL_UP_NAME, cGate::Type::INPUT, -1);
        case isldirection::Direction::ISL_RIGHT:
            return satelliteModule->gateHalf(Constants::ISL_RIGHT_NAME, cGate::Type::INPUT, -1);
        case isldirection::Direction::ISL_DOWN:
            return satelliteModule->gateHalf(Constants::ISL_DOWN_NAME, cGate::Type::INPUT, -1);
        case isldirection::Direction::ISL_DOWNLINK:
            return satelliteModule->gateHalf(Constants::SAT_GROUNDLINK_NAME, cGate::Type::INPUT, index);
    }
}

cGate* SatelliteInfo::getOutputGate(isldirection::Direction direction, int index) const {
    switch (direction) {
        case isldirection::Direction::ISL_LEFT:
            return satelliteModule->gateHalf(Constants::ISL_LEFT_NAME, cGate::Type::OUTPUT, -1);
        case isldirection::Direction::ISL_UP:
            return satelliteModule->gateHalf(Constants::ISL_UP_NAME, cGate::Type::OUTPUT, -1);
        case isldirection::Direction::ISL_RIGHT:
            return satelliteModule->gateHalf(Constants::ISL_RIGHT_NAME, cGate::Type::OUTPUT, -1);
        case isldirection::Direction::ISL_DOWN:
            return satelliteModule->gateHalf(Constants::ISL_DOWN_NAME, cGate::Type::OUTPUT, -1);
        case isldirection::Direction::ISL_DOWNLINK:
            return satelliteModule->gateHalf(Constants::SAT_GROUNDLINK_NAME, cGate::Type::OUTPUT, index);
    }
}

int SatelliteInfo::getPlane() const {
    int satsPerPlane = noradModule->getSatellitesPerPlane();
    int satPlane = (satelliteId - (satelliteId % satsPerPlane)) / satsPerPlane;
    ASSERT(satPlane < noradModule->getNumberOfPlanes());
    ASSERT(satPlane >= 0);
    return satPlane;
}

int SatelliteInfo::getNumberInPlane() const {
    int satsPerPlane = noradModule->getSatellitesPerPlane();
    int numberInPlane = satelliteId % satsPerPlane;
    ASSERT(numberInPlane >= 0);
    ASSERT(numberInPlane < satsPerPlane);
    return numberInPlane;
}

double SatelliteInfo::getLongitude() const {
    return noradModule->getLongitude();
}

double SatelliteInfo::getLatitude() const {
    return noradModule->getLatitude();
}

double SatelliteInfo::getAltitude() const {
    return noradModule->getAltitude();
}

double SatelliteInfo::getElevation(const PositionAwareBase& other) const {
    return ((INorad*)noradModule)->getElevation(other.getLatitude(), other.getLongitude(), other.getAltitude());
}

double SatelliteInfo::getAzimuth(const PositionAwareBase& other) const {
    return ((INorad*)noradModule)->getAzimuth(other.getLatitude(), other.getLongitude(), other.getAltitude());
}

bool SatelliteInfo::isAscending() const {
    return noradModule->isAscending();
}

bool SatelliteInfo::isDescending() const {
    return !noradModule->isAscending();
}

std::string to_string(const SatelliteInfo& satInfo) {
    std::ostringstream ss;
    ss << satInfo;
    return ss.str();
}

}  // namespace topologycontrol
}  // namespace flora
