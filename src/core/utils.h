/*
 * PacketGenerator.h
 *
 *  Created on: Mar 23, 2023
 *      Author: Robin Ohs
 */

#ifndef __FLORA_CORE_UTILS_H_
#define __FLORA_CORE_UTILS_H_

#include <omnetpp.h>

namespace flora {
namespace core {
namespace utils {

/** @brief Gets a random number from the interval [start, end] that is not equal to the excluded number. */
int randomNumber(omnetpp::cModule* mod, int start, int end, int excluded);

}  // namespace utils
}  // namespace core
}  // namespace flora

#endif  // __FLORA_CORE_UTILS_H_