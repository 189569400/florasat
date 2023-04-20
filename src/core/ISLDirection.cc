/*
 * ISLDirection.cc
 *
 * Created on: Apr 20, 2023
 *     Author: Robin Ohs
 */

#include "ISLDirection.h"

namespace flora {
namespace core {
namespace isldirection {

Direction getCounterDirection(Direction dir) {
    switch (dir) {
        case Direction::ISL_LEFT:
            return Direction::ISL_RIGHT;
        case Direction::ISL_UP:
            return Direction::ISL_DOWN;
        case Direction::ISL_RIGHT:
            return Direction::ISL_LEFT;
        case Direction::ISL_DOWN:
            return Direction::ISL_UP;
        default:
            throw omnetpp::cRuntimeError("Error in ISLDrection::connectToSatellite: (%d) %s is not supported here.", dir, to_string(dir).c_str());
    }
}

std::ostream &operator<<(std::ostream &ss, const Direction &direction) {
    switch (direction) {
        case Direction::ISL_LEFT:
            ss << Constants::ISL_LEFT_NAME;
        case Direction::ISL_UP:
            ss << Constants::ISL_UP_NAME;
        case Direction::ISL_RIGHT:
            ss << Constants::ISL_UP_NAME;
        case Direction::ISL_DOWN:
            ss << Constants::ISL_DOWN_NAME;
        case Direction::ISL_DOWNLINK:
            ss << Constants::SAT_GROUNDLINK_NAME;
    }
    return ss;
}

std::string to_string(const Direction &dir) {
    std::ostringstream ss;
    ss << dir;
    return ss.str();
}

}  // namespace isldirection
}  // namespace core
}  // namespace flora