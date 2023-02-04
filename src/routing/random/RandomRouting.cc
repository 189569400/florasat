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
        return ISLDirection::UP;
    }
}