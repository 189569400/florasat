/*
 * ISLState.h
 *
 * Created on: Mai 15, 2023
 *     Author: Robin Ohs
 */

#ifndef __FLORA_CORE_ISLSTATE_H_
#define __FLORA_CORE_ISLSTATE_H_

#include <omnetpp.h>

#include "Constants.h"

namespace flora {
namespace core {

enum ISLState {
    WORKING,
    DISABLED,
};

std::ostream &operator<<(std::ostream &ss, const ISLState &state);

std::string to_string(const ISLState &state);

}  // namespace core
}  // namespace flora

#endif  // __FLORA_CORE_ISLSTATE_H_