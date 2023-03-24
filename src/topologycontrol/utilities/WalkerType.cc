/*
 * WalkerType.cc
 *
 * Created on: Jan 24, 2023
 *     Author: Robin Ohs
 */

#include "WalkerType.h"

namespace flora {
namespace topologycontrol {
namespace WalkerType {

WalkerType parseWalkerType(std::string value) {
    if (value == "DELTA") {
        return WalkerType::DELTA;
    } else if (value == "STAR") {
        return WalkerType::STAR;
    }
    throw cRuntimeError("Error in WalkerType.parseWalkerType(): Could not find provided WalkerType.");
}

std::string as_string(WalkerType walkerType) {
    switch (walkerType) {
        case DELTA:
            return "DELTA";
        case STAR:
            return "STAR";
        default:
            throw cRuntimeError("Error in WalkerType.parseWalkerType(): Could not find provided WalkerType or is uninitialized.");
    }
}

}  // namespace WalkerType
}  // namespace topologycontrol
}  // namespace flora