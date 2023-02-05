/*
 * WalkerType.h
 *
 * Created on: Jan 24, 2023
 *     Author: Robin Ohs
 */

#ifndef TOPOLOGYCONTROL_COMMON_WALKERTYPE_H
#define TOPOLOGYCONTROL_COMMON_WALKERTYPE_H

#include <string.h>
#include <omnetpp.h>

using namespace omnetpp;

namespace flora
{

    namespace WalkerType
    {
        /** @brief The type of the walker constellation. */
        enum WalkerType
        {
            UNINITIALIZED,
            DELTA,
            STAR,
        };

        WalkerType parseWalkerType(std::string value);

        std::string as_string(WalkerType walkerType);
    } // walkertype

} // flora

#endif