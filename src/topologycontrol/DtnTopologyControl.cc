/*
 * TopologyControl.cc
 *
 * Created on: Dec 20, 2022
 *     Author: Robin Ohs
 */

#include "DtnTopologyControl.h"

namespace flora {
namespace topologycontrol {

Define_Module(DtnTopologyControl);

void DtnTopologyControl::initialize(int stage) {
    TopologyControlBase::initialize(stage);
}

void DtnTopologyControl::updateTopology() {
    core::Timer timer = core::Timer();
    // update ISL links and groundlinks
    topologyChanged = false;
    // updateIntraSatelliteLinks();
    updateInterSatelliteLinks();
    updateGroundstationLinksDtn();

    EV << "TC: Calculation took " << timer.getTime() / 1000 / 1000 << "ms" << endl;
    // if there was any change to the topology, track current contacts
    if (topologyChanged)
        trackTopologyChange();
}

void DtnTopologyControl::updateIntraSatelliteLinks() {
    ContactPlan *contactPlan = check_and_cast<ContactPlan *>(getParentModule()->getSubmodule("contactPlan"));
    ASSERT(contactPlan != nullptr);
    // iterate over planes
    for (size_t plane = 0; plane < planeCount; plane++) {
        // iterate over sats in plane
        for (size_t planeSat = 0; planeSat < satsPerPlane; planeSat++) {
            int index = planeSat + plane * satsPerPlane;
            int isLastSatInPlane = index % satsPerPlane == satsPerPlane - 1;

            // get the two satellites we want to connect.
            // If we have the last in plane, we connect it to the first of the plane
            SatelliteRoutingBase *curSat = satellites.at(index);
            ASSERT(curSat != nullptr);
            int nextId = isLastSatInPlane ? plane * satsPerPlane : index + 1;
            SatelliteRoutingBase *otherSat = satellites.at(nextId);
            ASSERT(otherSat != nullptr);

            vector<Contact> contactsBetweenSats = contactPlan->getContactsBySrcDst(numGroundStations + curSat->getId(), numGroundStations + otherSat->getId());
            for (size_t satContactIndex = 0; satContactIndex < contactsBetweenSats.size(); satContactIndex++) {
                int shiftedCurSatId = getShiftedSatelliteId(curSat->getId());
                int shiftedOtherSatId = getShiftedSatelliteId(otherSat->getId());
                if (isDtnContactStarting(shiftedCurSatId, shiftedOtherSatId, contactsBetweenSats.at(satContactIndex))){
                    connectSatellites(curSat, otherSat, isldirection::Direction::ISL_UP);
                }
            }
        }
    }
}

/**
 * Updates the links between each Groundstation and Satellites based on a ContactPlan instance.
 */
void DtnTopologyControl::updateGroundstationLinksDtn() {
    ContactPlan *contactPlan = check_and_cast<ContactPlan *>(getParentModule()->getSubmodule("contactPlan"));
    if (contactPlan == nullptr) {
        error("Error in TopologyControl::updateNodeLinksDtn(): contactPlan is nullptr. Make sure the module exists.");
    }
    // TODO: Improve time complexity in this function from O(n^3)
    for (size_t gsId = 0; gsId < numGroundStations; gsId++) {
        for (size_t satId = 0; satId < numSatellites; satId++) {
            vector<Contact> satContacts = contactPlan->getContactsBySrcDst(gsId, satId + numGroundStations);
            for (size_t i = 0; i < satContacts.size(); i++) {
                Contact contact = satContacts.at(i);
                int shiftedSatId = getShiftedSatelliteId(satId);
                if (isDtnContactStarting(gsId, shiftedSatId, contact)){
                    linkGroundStationToSatDtn(gsId, shiftedSatId);
                } else if (isDtnContactTakingPlace(gsId, shiftedSatId, contact)) {
                    updateLinkGroundStationToSatDtn(gsId, shiftedSatId);
                } else if (isDtnContactEnding(gsId, shiftedSatId, contact)) {
                    unlinkGroundStationToSatDtn(gsId, shiftedSatId);
                }
            }
        }
    }
}

/**
 * Checks if a contact between a GroundStation and Satellite is starting based on a Contact instance
 *
 * @param gsInfo contains data related to a GroundStation
 * @param satInfo contains data related to a Satellite
 * @param contact represent a Contact between two nodes in a Contact Plan
 * @return whether a contact between a GroundStation and a Satellite is starting
 */
bool DtnTopologyControl::isDtnContactStarting(int gsId, int satId, Contact contact) { // TODO: Change name of gsId since it can be satId also
    return contact.getSourceEid() == gsId && contact.getDestinationEid() == satId && contact.getStart() == simTime().dbl();
}

/**
 * Checks if a contact between a GroundStation and Satellite is taking place based on a Contact instance
 *
 * @param gsInfo contains data related to a GroundStation
 * @param satInfo contains data related to a Satellite
 * @param contact represent a Contact between two nodes in a Contact Plan
 * @return whether a contact between a GroundStation and a Satellite is taking place
 */
bool DtnTopologyControl::isDtnContactTakingPlace(int gsId, int satId, Contact contact) {
    return contact.getSourceEid() == gsId && contact.getDestinationEid() == satId && contact.getEnd() > simTime().dbl() && contact.getStart() < simTime().dbl();
}

/**
 * Checks if a contact between a GroundStation and Satellite is ending based on a Contact instance
 *
 * @param gsInfo contains data related to a GroundStation
 * @param satInfo contains data related to a Satellite
 * @param contact represent a Contact between two nodes in a Contact Plan
 * @return whether a contact between a GroundStation and a Satellite is ending
 */
bool DtnTopologyControl::isDtnContactEnding(int gsId, int satId, Contact contact) {
    return contact.getSourceEid() == gsId && contact.getDestinationEid() == satId && contact.getEnd() == simTime().dbl();
}

/**
 * Creates a link between a GroundStation and Satellite
 * @param gsInfo contains data related to a GroundStation
 * @param satInfo contains data related to a Satellite
 */
void DtnTopologyControl::linkGroundStationToSatDtn(int gsId, int satId) {
    GroundStationRoutingBase *gs = groundStations.at(gsId);
    SatelliteRoutingBase *sat = satellites.at(satId);
    double delay = sat->getDistance(*gs) * groundlinkDelay; // delay of the channel between satellite and groundstation (ms)
    EV << "Create channel between GS " << gsId << " and SAT " << satId << endl;
    int freeIndexGs = -1;
    for (size_t i = 0; i < numGroundLinks; i++) {
        cGate *gate = gs->getOutputGate(i);
        if (!gate->isConnectedOutside()) {
            freeIndexGs = i;
            break;
        }
    }
    if (freeIndexGs == -1) {
        error("No free gs gate index found.");
    }

    int freeIndexSat = -1;
    for (size_t i = 0; i < numGroundLinks; i++) {
        const cGate *gate = sat->getOutputGate(isldirection::Direction::ISL_DOWNLINK, i).first;
        if (!gate->isConnectedOutside()) {
            freeIndexSat = i;
            break;
        }
    }
    if (freeIndexSat == -1) {
        error("No free sat gate index found.");
    }

    cGate *uplinkO = gs->getOutputGate(freeIndexGs);
    cGate *uplinkI = gs->getInputGate(freeIndexGs);
    cGate *downlinkO = sat->getOutputGate(isldirection::Direction::ISL_DOWNLINK, freeIndexSat).first;
    cGate *downlinkI = sat->getInputGate(isldirection::Direction::ISL_DOWNLINK, freeIndexSat).first;
    updateOrCreateChannel(uplinkO, downlinkI, delay, groundlinkDatarate);
    updateOrCreateChannel(downlinkO, uplinkI, delay, groundlinkDatarate);
    gsSatConnections.emplace(std::pair<int, int>(gsId, satId), GsSatConnection(gsId, satId, freeIndexGs, freeIndexSat));
    gs->addSatellite(satId);
    topologyChanged = true;
}

/**
 * Deletes the link between a GroundStation and Satellite
 * @param gsInfo contains data related to a GroundStation
 * @param satInfo contains data related to a Satellite
 */
void DtnTopologyControl::unlinkGroundStationToSatDtn(int gsId, int satId) {
    GroundStationRoutingBase *gs = groundStations.at(gsId);
    SatelliteRoutingBase *sat = satellites.at(satId);
    GsSatConnection &connection = gsSatConnections.at(std::pair<int, int>(gsId, satId));
    cGate *uplink= gs->getOutputGate(connection.gsGateIndex);
    cGate *downlink = sat->getOutputGate(isldirection::Direction::ISL_DOWNLINK, connection.satGateIndex).first;
    deleteChannel(uplink);
    deleteChannel(downlink);
    gsSatConnections.erase(std::pair<int, int>(gsId, satId));
    gs->removeSatellite(satId);
    topologyChanged = true;
}

/**
 * Update the link between a GroundStation and Satellite
 * @param gsInfo contains data related to a GroundStation
 * @param satInfo contains data related to a Satellite
 */
void DtnTopologyControl::updateLinkGroundStationToSatDtn(int gsId, int satId) {
    GroundStationRoutingBase *gs = groundStations.at(gsId);
    SatelliteRoutingBase *sat = satellites.at(satId);
    double delay = sat->getDistance(*gs) * groundlinkDelay; // delay of the channel between nearest satellite and groundstation (ms)
    GsSatConnection &connection = gsSatConnections.at(std::pair<int, int>(gsId, satId));
    cGate *uplinkO = gs->getOutputGate(connection.gsGateIndex);
    cGate *uplinkI = gs->getInputGate(connection.gsGateIndex);
    cGate *downlinkO = sat->getOutputGate(isldirection::Direction::ISL_DOWNLINK, connection.satGateIndex).first;
    cGate *downlinkI = sat->getInputGate(isldirection::Direction::ISL_DOWNLINK, connection.satGateIndex).first;
    updateOrCreateChannel(uplinkO, downlinkI, delay, groundlinkDatarate);
    updateOrCreateChannel(downlinkO, uplinkI, delay, groundlinkDatarate);
}

int DtnTopologyControl::getShiftedSatelliteId(int satId) {
    return numGroundStations + satId;
}

void DtnTopologyControl::updateInterSatelliteLinks() {
    // if inter-plane ISL is not enabled/available
    if (interPlaneIslDisabled) return;

    ContactPlan *contactPlan = check_and_cast<ContactPlan *>(getParentModule()->getSubmodule("contactPlan"));
    ASSERT(contactPlan != nullptr);

    for (size_t satId = 0; satId < numSatellites; satId++) {
        vector<Contact> satContacts = contactPlan->getContactsBySrc(getShiftedSatelliteId(satId));
        for (size_t contactIndex = 0; contactIndex < satContacts.size(); contactIndex++) {
            Contact contact = satContacts.at(contactIndex);
            if (contact.getSourceEid() >= numGroundStations && contact.getDestinationEid() >= numGroundStations) {
                SatelliteRoutingBase *curSat = satellites.at(contact.getSourceEid()- numGroundStations);
                SatelliteRoutingBase *otherSat = satellites.at(contact.getDestinationEid()- numGroundStations);
                if (isDtnContactStarting(contact.getSourceEid(), contact.getDestinationEid(), contact)) {
                    if (curSat->isAscending()) {
                        connectSatellites(curSat, otherSat, isldirection::Direction::ISL_RIGHT);
                    } else {
                        connectSatellites(curSat, otherSat, isldirection::Direction::ISL_LEFT);
                    }
                } else if (isDtnContactEnding(contact.getSourceEid(), contact.getDestinationEid(), contact)) {
                    if (curSat->hasRightSat() && curSat->getRightSatId() == otherSat->getId()) {
                        disconnectSatellites(curSat, otherSat, isldirection::Direction::ISL_RIGHT);
                    } else if (curSat->hasLeftSat() && curSat->getLeftSatId() == otherSat->getId()) {
                        disconnectSatellites(curSat, otherSat, isldirection::Direction::ISL_LEFT);
                    }
                }
            }
        }
    }
}

}  // namespace topologycontrol
}  // namespace flora
