/*
 * SatelliteInfo.cc
 *
 * Created on: Feb 10, 2023
 *     Author: Robin Ohs
 */

#include "SatelliteInfo.h"

#define NOT_SET -1;

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

void SatelliteInfo::setLeftSat(const SatelliteInfo& newLeft) {
    leftSatellite = newLeft.getSatelliteId();
    leftDistance = getDistance(newLeft);
}
void SatelliteInfo::setUpSat(const SatelliteInfo& newUp) {
    upSatellite = newUp.getSatelliteId();
    upDistance = getDistance(newUp);
}
void SatelliteInfo::setRightSat(const SatelliteInfo& newRight) {
    rightSatellite = newRight.getSatelliteId();
    rightDistance = getDistance(newRight);
}
void SatelliteInfo::setDownSat(const SatelliteInfo& newDown) {
    downSatellite = newDown.getSatelliteId();
    downDistance = getDistance(newDown);
}

void SatelliteInfo::removeLeftSat() {
    leftSatellite = NOT_SET;
    leftDistance = NOT_SET;
}
void SatelliteInfo::removeUpSat() {
    upSatellite = NOT_SET;
    upDistance = NOT_SET;
}
void SatelliteInfo::removeRightSat() {
    rightSatellite = NOT_SET;
    rightDistance = NOT_SET;
}
void SatelliteInfo::removeDownSat() {
    downSatellite = NOT_SET;
    downDistance = NOT_SET;
}

bool SatelliteInfo::hasLeftSat() const {
    return leftSatellite != NOT_SET;
}
bool SatelliteInfo::hasUpSat() const {
    return upSatellite != NOT_SET;
}
bool SatelliteInfo::hasRightSat() const {
    return rightSatellite != NOT_SET;
}
bool SatelliteInfo::hasDownSat() const {
    return downSatellite != NOT_SET;
}

int SatelliteInfo::getLeftSat() const {
    VALIDATE_SET(leftSatellite);
    return leftSatellite;
}
int SatelliteInfo::getUpSat() const {
    VALIDATE_SET(upSatellite);
    return upSatellite;
}
int SatelliteInfo::getRightSat() const {
    VALIDATE_SET(rightSatellite);
    return rightSatellite;
}
int SatelliteInfo::getDownSat() const {
    VALIDATE_SET(downSatellite);
    return downSatellite;
}

double SatelliteInfo::getLeftSatDistance() const {
    VALIDATE_SET(leftDistance);
    return leftDistance;
}
double SatelliteInfo::getUpSatDistance() const {
    VALIDATE_SET(upSatellite);
    return upDistance;
}
double SatelliteInfo::getRightSatDistance() const {
    VALIDATE_SET(rightDistance);
    return rightDistance;
}
double SatelliteInfo::getDownSatDistance() const {
    VALIDATE_SET(downDistance);
    return downDistance;
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
