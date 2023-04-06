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
    } else if (stage == inet::INITSTAGE_ROUTING_PROTOCOLS) {
        satMobVector = loadSatMobilities();
        updatePositions();
        if (!updateTimer->isScheduled()) {
            scheduleUpdate();
        }
    }
}

SatMobVector FasterSatMobility::loadSatMobilities() {
    cModule *parent = getParentModule();
    if (parent == nullptr) {
        error("Error in FasterSatMobility::loadSatellites: Could not find parent.");
    }
    int numSats = parent->getSubmoduleVectorSize("loRaGW");
    SatMobVector satellites;
    for (size_t i = 0; i < numSats; i++) {
        cModule *sat = parent->getSubmodule("loRaGW", i);
        if (sat == nullptr) {
            error("Error in FasterSatMobility::loadSatellites: Could not find sat with id %d.", i);
        }
        inet::SatelliteMobility *satMobility = check_and_cast<inet::SatelliteMobility *>(sat->getSubmodule("mobility"));
        if (satMobility == nullptr) {
            error("Error in FasterSatMobility::loadSatellites: Could not find sat mobility for sat %d.", i);
        }
        satellites.emplace_back(satMobility);
    }
    return satellites;
}

// int FasterSatMobility::calculateSliceCount(int satMobilityCount) {
//     return 12;
// }

// void FasterSatMobility::createSatSlices(int sliceCount, Slice slice) {
//     // fill the map with empty vectors to store the slices
//     for (size_t i = 0; i < sliceCount; i++) {
//         Slice t;
//         satMobilitySlices.emplace(i, t);
//     }
//     // distribute ptrs to the sat mobilities between all slices
//     size_t index = 0;
//     while (!slice.empty()) {
//         inet::SatelliteMobility* satMobility = slice.back();
//         slice.pop_back();
//         satMobilitySlices.at(index).push_back(satMobility);
//         index = ++index % sliceCount;
//     }
// }

void FasterSatMobility::handleMessage(cMessage *msg) {
    if (msg == updateTimer) {
        updatePositions();
        scheduleUpdate();
    } else
        error("FasterSatMobility: Unknown message.");
}

void FasterSatMobility::updatePositions() {
    EV << "Update satellite positions." << endl;
    simtime_t currentTime = simTime();
    core::Timer timer = core::Timer();
    for (auto sm : satMobVector) {
        sm->updatePosition();
    }
    EV << "FSM: Calculation took " << timer.getTime() / 1000 / 1000 << "ms" << endl;
}

void FasterSatMobility::scheduleUpdate() {
    scheduleClockEventAfter(updateIntervalParameter->doubleValue(), updateTimer);
}

}  // namespace mobility
}  // namespace flora