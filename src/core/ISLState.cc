/*
 * ISLState.cc
 *
 * Created on: Apr 20, 2023
 *     Author: Robin Ohs
 */

#include "ISLState.h"

namespace flora {
namespace core {

std::ostream &operator<<(std::ostream &ss, const ISLState &state) {
    switch (state) {
        case ISLState::NORMAL:
            ss << Constants::ISL_STATE_NORMAL;
            break;
        default:
            throw omnetpp::cRuntimeError("Error in ISLDrection::string operator");
    }
    return ss;
}

std::string to_string(const ISLState &state) {
    std::ostringstream ss;
    ss << state;
    return ss.str();
}

}  // namespace core
}  // namespace flora