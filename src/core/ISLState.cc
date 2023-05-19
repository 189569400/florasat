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
        case ISLState::WORKING:
            ss << Constants::ISL_STATE_WORKING;
            break;
        default:
            throw omnetpp::cRuntimeError("Error in ISLState::string operator");
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