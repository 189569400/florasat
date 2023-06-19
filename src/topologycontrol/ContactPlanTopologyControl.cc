/*
 * TopologyControl.cc
 *
 * Created on: Dec 20, 2022
 *     Author: Robin Ohs
 */

#include "ContactPlanTopologyControl.h"
#include <cmath>

namespace flora {
namespace topologycontrol {

Define_Module(ContactPlanTopologyControl);

void ContactPlanTopologyControl::initialize(int stage) {
    TopologyControlBase::initialize(stage);
    distanceMode = par("distanceMode").stdstringValue();
}

void ContactPlanTopologyControl::updateTopology() {
    core::Timer timer = core::Timer();
    // update ISL links and groundlinks
    topologyChanged = false;
    updateIntraSatelliteLinks();
    updateInterSatelliteLinks();
    updateGroundstationLinksDtn();

    EV << "TC: Calculation took " << timer.getTime() / 1000 / 1000 << "ms" << endl;
    // if there was any change to the topology, track current contacts
    if (topologyChanged)
        trackTopologyChange();
}


/**
 * Updates the links between each Groundstation and Satellites based on a ContactPlan instance.
 */
void ContactPlanTopologyControl::updateGroundstationLinksDtn() {
    // TODO: Implement custom logic
    EV << "updateGroundstationLinksDtn: starting" << endl;
    ContactPlan *contactPlan = check_and_cast<ContactPlan *>(getParentModule()->getSubmodule("contactPlan"));
    ASSERT(contactPlan != nullptr);
    for (size_t gsId = 0; gsId < numGroundStations; gsId++) {
        GroundStationRoutingBase *gs = groundStations.at(gsId);
        for (size_t satId = 0; satId < numSatellites; satId++) {
            SatelliteRoutingBase *sat = satellites.at(satId);
            if (isGroundStationContactValid(gs, sat) && contactStarts.count(std::pair<int, int>(gsId, getShiftedSatelliteId(satId))) == 0){
                EV << "Starting Contact: " << endl;
                contactStarts.emplace(std::pair<int, int>(gsId, getShiftedSatelliteId(satId)), simTime().dbl());
            } else if (!isGroundStationContactValid(gs, sat) && contactStarts.count(std::pair<int, int>(gsId, getShiftedSatelliteId(satId))) > 0) {
                EV << "Ending Contact: " << endl;
                double startTime = contactStarts.at(std::pair<int, int>(gsId, getShiftedSatelliteId(satId)));
                double endTime = simTime().dbl();
                contactPlan->addContact(startTime, endTime, gsId, getShiftedSatelliteId(satId), 1000, 1);
                contactPlan->addContact(startTime, endTime, getShiftedSatelliteId(satId), gsId, 1000, 1);
                contactPlan->addRange(startTime, endTime, gsId, getShiftedSatelliteId(satId), 1000, 1);
                contactStarts.erase(std::pair<int, int>(gsId, getShiftedSatelliteId(satId)));
            }
        }
    }
    contactPlan->updateContactRanges();
    contactPlan->sortContactIdsBySrcByStartTime();
    contactPlan->printContactPlan();
}

bool ContactPlanTopologyControl::isGroundStationContactValid(GroundStationRoutingBase *gs, SatelliteRoutingBase *sat){
    if (distanceMode == "dummy") {
        double distance = sat->getDistance(*gs);
        // EV << "Distance: " << distance << " (KM)" << endl;
        return distance < 7000.0;
    }
    return false;
}

void ContactPlanTopologyControl::updateIntraSatelliteLinks() {
    // TODO: Implement custom logic
    EV << "updateIntraSatelliteLinks: starting" << endl;

}

void ContactPlanTopologyControl::updateInterSatelliteLinks() {
   // TODO: Implement custom logic
    EV << "updateInterSatelliteLinks: starting" << endl;
}

int ContactPlanTopologyControl::getShiftedSatelliteId(int satId) {
    return numGroundStations + satId;
}

}  // namespace topologycontrol
}  // namespace flora
