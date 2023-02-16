/*
 * FasterSatMobility.cc
 *
 *  Created on: Feb 16, 2023
 *      Author: Robin Ohs
 */

#include "FasterSatMobility.h"

namespace flora
{
    Define_Module(FasterSatMobility);

    FasterSatMobility::FasterSatMobility() : updatePos(nullptr),
                                             updateIntervalPos(0)
    {
    }

    FasterSatMobility::~FasterSatMobility()
    {
        cancelAndDelete(updatePos);
    }

    void FasterSatMobility::initialize(int stage)
    {
        if (stage == 0)
        {
            updateIntervalPos = par("updateIntervalPos");
        }
        else if (stage == inet::INITSTAGE_APPLICATION_LAYER)
        {
            updatePos = new cMessage(POSITION, 1);
            updatePositions();
        }
    }

    void FasterSatMobility::handleMessage(cMessage *msg)
    {
        if (msg->getKind() == 1)
            updatePositions();
    }

    void FasterSatMobility::updatePositions()
    {
        EV << "Update satellite positions" << endl;
        cModule *parent = getParentModule();
        if (parent == nullptr)
        {
            error("Error in FasterSatMobility::updatePositions: Could not find parent.");
        }
        int numSats = parent->getSubmoduleVectorSize("loRaGW");
        for (size_t i = 0; i < numSats; i++)
        {
            cModule *sat = parent->getSubmodule("loRaGW", i);
            if (sat == nullptr)
            {
                error("Error in FasterSatMobility::updateSat: Could not find sat with id %d.", i);
            }
            inet::SatelliteMobility *satMobility = check_and_cast<inet::SatelliteMobility *>(sat->getSubmodule("mobility"));
            if (satMobility == nullptr)
            {
                error("Error in FasterSatMobility::updateSat: Could not find sat mobility for sat %d.", i);
            }
            satMobility->updatePosition();
        }
        cancelEvent(updatePos);
        if (updateIntervalPos != 0)
        {
            simtime_t nextUpdate = simTime() + updateIntervalPos;
            scheduleAt(nextUpdate, updatePos);
        }
    }
} // namespace flora