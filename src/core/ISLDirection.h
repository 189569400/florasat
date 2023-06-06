/*
 * ISLDirection.h
 *
 * Created on: Feb 04, 2023
 *     Author: Robin Ohs
 */

#ifndef __FLORA_CORE_ISLDIRECTION_ISLDIRECTION_H_
#define __FLORA_CORE_ISLDIRECTION_ISLDIRECTION_H_

#include <omnetpp.h>

#include "core/Constants.h"

using namespace omnetpp;

namespace flora {
namespace core {
namespace isldirection {

enum ISLDirection {
    LEFT,
    UP,
    RIGHT,
    DOWN,
    GROUNDLINK
};

ISLDirection getCounterDirection(ISLDirection dir);

ISLDirection from_str(const char* text);

std::ostream &operator<<(std::ostream &ss, const ISLDirection &direction);

std::string to_string(const ISLDirection &dir);

}  // namespace isldirection
}  // namespace core
}  // namespace flora

#endif  // __FLORA_CORE_ISLDIRECTION_ISLDIRECTION_H_