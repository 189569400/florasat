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

ISLDirection getCounterDirection(ISLDirection dir) {
    switch (dir) {
        case ISLDirection::LEFT:
            return ISLDirection::RIGHT;
        case ISLDirection::UP:
            return ISLDirection::DOWN;
        case ISLDirection::RIGHT:
            return ISLDirection::LEFT;
        case ISLDirection::DOWN:
            return ISLDirection::UP;
        default:
            throw omnetpp::cRuntimeError("Error in ISLDrection::connectToSatellite: (%d) %s is not supported here.", dir, to_string(dir).c_str());
    }
}

ISLDirection from_str(const char *text) {
    if (!strcasecmp(text, Constants::ISL_LEFT_NAME))
        return ISLDirection::LEFT;
    else if (!strcasecmp(text, Constants::ISL_UP_NAME))
        return ISLDirection::UP;
    else if (!strcasecmp(text, Constants::ISL_RIGHT_NAME))
        return ISLDirection::RIGHT;
    else if (!strcasecmp(text, Constants::ISL_DOWN_NAME))
        return ISLDirection::DOWN;
    else if (!strcasecmp(text, Constants::SAT_GROUNDLINK_NAME))
        return ISLDirection::GROUNDLINK;
    else
        throw cRuntimeError("Unknown text constant: %s", text);
}

std::ostream &operator<<(std::ostream &ss, const ISLDirection &direction) {
    switch (direction) {
        case ISLDirection::LEFT:
            ss << Constants::ISL_LEFT_NAME;
            break;
        case ISLDirection::UP:
            ss << Constants::ISL_UP_NAME;
            break;
        case ISLDirection::RIGHT:
            ss << Constants::ISL_RIGHT_NAME;
            break;
        case ISLDirection::DOWN:
            ss << Constants::ISL_DOWN_NAME;
            break;
        case ISLDirection::GROUNDLINK:
            ss << Constants::SAT_GROUNDLINK_NAME;
            break;
        default:
            throw omnetpp::cRuntimeError("Error in ISLDrection::string operator");
    }
    return ss;
}

std::string to_string(const ISLDirection &dir) {
    std::ostringstream ss;
    ss << dir;
    return ss.str();
}

}  // namespace isldirection
}  // namespace core
}  // namespace flora