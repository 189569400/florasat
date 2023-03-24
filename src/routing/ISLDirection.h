/*
 * Direction.h
 *
 * Created on: Feb 04, 2023
 *     Author: Robin Ohs
 */

#ifndef __FLORA_ROUTING_ISLDIRECTION_H_
#define __FLORA_ROUTING_ISLDIRECTION_H_

#include <omnetpp.h>

namespace flora {

enum Direction {
    ISL_LEFT,
    ISL_UP,
    ISL_RIGHT,
    ISL_DOWN,
    ISL_DOWNLINK
};

struct ISLDirection {
    Direction direction;
    int gateIndex;

    ISLDirection(Direction direction, int gateIndex)
        : direction(direction),
          gateIndex(gateIndex){};
};

}  // namespace flora

#endif  // __FLORA_ROUTING_ISLDIRECTION_H_