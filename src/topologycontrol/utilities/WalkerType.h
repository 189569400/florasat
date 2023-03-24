/*
 * WalkerType.h
 *
 * Created on: Jan 24, 2023
 *     Author: Robin Ohs
 */

#ifndef __FLORA_TOPOLOGYCONTROL_UTILITIES_WALKERTYPE_H_
#define __FLORA_TOPOLOGYCONTROL_UTILITIES_WALKERTYPE_H_

#include <omnetpp.h>
#include <string.h>

using namespace omnetpp;

namespace flora {
namespace topologycontrol {
namespace WalkerType {

/** @brief The type of the walker constellation. */
enum WalkerType {
    UNINITIALIZED,
    DELTA,
    STAR,
};

WalkerType parseWalkerType(std::string value);

std::string as_string(WalkerType walkerType);

}  // namespace WalkerType
}  // namespace topologycontrol
}  // namespace flora

#endif // __FLORA_TOPOLOGYCONTROL_UTILITIES_WALKERTYPE_H_