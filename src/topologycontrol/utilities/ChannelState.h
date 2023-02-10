/*
 * ChannelState.h
 *
 * Created on: Feb 10, 2023
 *     Author: Robin Ohs
 */

#ifndef TOPOLOGYCONTROL_UTILITIES_CHANNELSTATE_H
#define TOPOLOGYCONTROL_UTILITIES_CHANNELSTATE_H

#include <string.h>
#include <omnetpp.h>

using namespace omnetpp;

namespace flora
{
    /** @brief Used to indicate if there was a state change to a channel. UPDATED is equal to UNCHANGED. */
    enum ChannelState
    {
        CREATED,
        DELETED,
        UPDATED,
        UNCHANGED,
    };
} // flora

#endif