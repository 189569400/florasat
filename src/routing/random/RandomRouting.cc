/*
 * RandomRouting.cc
 *
 * Created on: Feb 04, 2023
 *     Author: Robin Ohs
 */

#include "RandomRouting.h"

namespace flora
{
    Define_Module(RandomRouting);

    ISLDirection RandomRouting::RoutePacket(cMessage *msg, cModule *callerSat)
    {
        int gate = intuniform(0, 3);

        if (gate == 0 && HasConnection(callerSat, ISLDirection::ISL_DOWN)) {
            return ISLDirection::ISL_DOWN;
        } else if (gate == 1 && HasConnection(callerSat, ISLDirection::ISL_UP)) {
            return ISLDirection::ISL_UP;
        } else if (gate == 2 && HasConnection(callerSat, ISLDirection::ISL_LEFT)) {
            return ISLDirection::ISL_LEFT;
        } else if (gate == 3 && HasConnection(callerSat, ISLDirection::ISL_RIGHT)) {
            return ISLDirection::ISL_RIGHT;
        }
        return RoutePacket(msg, callerSat);
    }
}