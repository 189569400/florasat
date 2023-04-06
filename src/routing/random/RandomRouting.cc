/*
 * RandomRouting.cc
 *
 * Created on: Feb 04, 2023
 *     Author: Robin Ohs
 */

#include "RandomRouting.h"

namespace flora {
namespace routing {

Define_Module(RandomRouting);

ISLDirection RandomRouting::RoutePacket(inet::Packet *pkt, cModule *callerSat) {
    int gate = core::utils::randomNumber(this, 0, 3, -1);

    if (gate == 0 && RoutingBase::HasConnection(callerSat, ISLDirection(Direction::ISL_DOWN, -1))) {
        return ISLDirection(Direction::ISL_DOWN, -1);
    } else if (gate == 1 && RoutingBase::HasConnection(callerSat, ISLDirection(Direction::ISL_UP, -1))) {
        return ISLDirection(Direction::ISL_UP, -1);
    } else if (gate == 2 && RoutingBase::HasConnection(callerSat, ISLDirection(Direction::ISL_LEFT, -1))) {
        return ISLDirection(Direction::ISL_LEFT, -1);
    } else if (gate == 3 && RoutingBase::HasConnection(callerSat, ISLDirection(Direction::ISL_RIGHT, -1))) {
        return ISLDirection(Direction::ISL_RIGHT, -1);
    }
    return RoutePacket(pkt, callerSat);
}

}  // namespace routing
}  // namespace flora