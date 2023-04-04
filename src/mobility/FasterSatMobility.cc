/*
 * FasterSatMobility.cc
 *
 *  Created on: Feb 16, 2023
 *      Author: Robin Ohs
 */

#include "FasterSatMobility.h"

namespace flora {
namespace mobility {

Define_Module(FasterSatMobility);

FasterSatMobility::FasterSatMobility() : updateTimer(nullptr),
                                         updateIntervalParameter(0) {
}

FasterSatMobility::~FasterSatMobility() {
    cancelAndDeleteClockEvent(updateTimer);
}

void FasterSatMobility::initialize(int stage) {
    if (stage == inet::INITSTAGE_LOCAL) {
        updateIntervalParameter = &par("updateInterval");
        updateTimer = new inet::ClockEvent("UpdateTimer");
    } else if (stage == inet::INITSTAGE_APPLICATION_LAYER) {
        updatePositions();
        if (!updateTimer->isScheduled()) {
            scheduleUpdate();
        }
    }
}

void FasterSatMobility::handleMessage(cMessage *msg) {
    if (msg == updateTimer) {
        updatePositions();
        scheduleUpdate();
    } else
        error("FasterSatMobility: Unknown message.");
}

void FasterSatMobility::updatePositions() {
    EV_DEBUG << "Update satellite positions" << endl;
    cModule *parent = getParentModule();
    if (parent == nullptr) {
        error("Error in FasterSatMobility::updatePositions: Could not find parent.");
    }
    int numSats = parent->getSubmoduleVectorSize("loRaGW");
    for (size_t i = 0; i < numSats; i++) {
        cModule *sat = parent->getSubmodule("loRaGW", i);
        if (sat == nullptr) {
            error("Error in FasterSatMobility::updateSat: Could not find sat with id %d.", i);
        }
        inet::SatelliteMobility *satMobility = check_and_cast<inet::SatelliteMobility *>(sat->getSubmodule("mobility"));
        if (satMobility == nullptr) {
            error("Error in FasterSatMobility::updateSat: Could not find sat mobility for sat %d.", i);
        }
        satMobility->updatePosition();
    }
}

void FasterSatMobility::scheduleUpdate() {
    scheduleClockEventAfter(updateIntervalParameter->doubleValue(), updateTimer);
}

}  // namespace mobility
}  // namespace flora