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

    // DTN related initialization
}

void DtnTopologyControl::updateTopology() {
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

void DtnTopologyControl::updateIntraSatelliteLinks() {
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

            // connect the satellites
            connectSatellites(curSat, otherSat, isldirection::Direction::ISL_UP);
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
                if (isDtnContactStarting(gsId, satId, contact)){
                    linkGroundStationToSatDtn(gsId, satId);
                } else if (isDtnContactTakingPlace(gsId, satId, contact)) {
                    updateLinkGroundStationToSatDtn(gsId, satId);
                } else if (isDtnContactEnding(gsId, satId, contact)) {
                    unlinkGroundStationToSatDtn(gsId, satId);
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
bool DtnTopologyControl::isDtnContactStarting(int gsId, int satId, Contact contact) {
    return contact.getSourceEid() == gsId && contact.getDestinationEid() == satId + numGroundStations && contact.getStart() == simTime().dbl();
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
    return contact.getSourceEid() == gsId && contact.getDestinationEid() == satId + numGroundStations && contact.getEnd() > simTime().dbl() && contact.getStart() < simTime().dbl();
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
    return contact.getSourceEid() == gsId && contact.getDestinationEid() == satId + numGroundStations && contact.getEnd() == simTime().dbl();
}

/**
 * Creates a link between a GroundStation and Satellite
 * @param gsInfo contains data related to a GroundStation
 * @param satInfo contains data related to a Satellite
 */
void DtnTopologyControl::linkGroundStationToSatDtn(int gsId, int satId) {
    GroundstationInfo &gsInfo = groundstationInfos.at(gsId);
    SatelliteRoutingBase *sat = satellites.at(satId);
    double delay = sat->getDistance(gsInfo) * groundlinkDelay; // delay of the channel between satellite and groundstation (ms)
    EV << "Create channel between GS " << gsId << " and SAT " << satId << endl;
    int freeIndexGs = -1;
    for (size_t i = 0; i < numGroundLinks; i++) {
        cGate *gate = gsInfo.getOutputGate(i);
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

    cGate *uplinkO = gsInfo.getOutputGate(freeIndexGs);
    cGate *uplinkI = gsInfo.getInputGate(freeIndexGs);
    cGate *downlinkO = sat->getOutputGate(isldirection::Direction::ISL_DOWNLINK, freeIndexSat).first;
    cGate *downlinkI = sat->getInputGate(isldirection::Direction::ISL_DOWNLINK, freeIndexSat).first;
    updateOrCreateChannel(uplinkO, downlinkI, delay, groundlinkDatarate);
    updateOrCreateChannel(downlinkO, uplinkI, delay, groundlinkDatarate);
    gsSatConnections.emplace(std::pair<int, int>(gsId, satId), GsSatConnection(gsId, satId, freeIndexGs, freeIndexSat));
    gsInfo.satellites.emplace(satId);
    topologyChanged = true;
}

/**
 * Deletes the link between a GroundStation and Satellite
 * @param gsInfo contains data related to a GroundStation
 * @param satInfo contains data related to a Satellite
 */
void DtnTopologyControl::unlinkGroundStationToSatDtn(int gsId, int satId) {
    GroundstationInfo &gsInfo = groundstationInfos.at(gsId);
    SatelliteRoutingBase *sat = satellites.at(satId);
    GsSatConnection &connection = gsSatConnections.at(std::pair<int, int>(gsId, satId));
    cGate *uplink= gsInfo.getOutputGate(connection.gsGateIndex);
    cGate *downlink = sat->getOutputGate(isldirection::Direction::ISL_DOWNLINK, connection.satGateIndex).first;
    deleteChannel(uplink);
    deleteChannel(downlink);
    gsSatConnections.erase(std::pair<int, int>(gsId, satId));
    gsInfo.satellites.erase(satId);
    topologyChanged = true;
}

/**
 * Update the link between a GroundStation and Satellite
 * @param gsInfo contains data related to a GroundStation
 * @param satInfo contains data related to a Satellite
 */
void DtnTopologyControl::updateLinkGroundStationToSatDtn(int gsId, int satId) {
    GroundstationInfo &gsInfo = groundstationInfos.at(gsId);
    SatelliteRoutingBase *sat = satellites.at(satId);
    double delay = sat->getDistance(gsInfo) * groundlinkDelay; // delay of the channel between nearest satellite and groundstation (ms)
    GsSatConnection &connection = gsSatConnections.at(std::pair<int, int>(gsId, satId));
    cGate *uplinkO = gsInfo.getOutputGate(connection.gsGateIndex);
    cGate *uplinkI = gsInfo.getInputGate(connection.gsGateIndex);
    cGate *downlinkO = sat->getInputGate(isldirection::Direction::ISL_DOWNLINK, connection.satGateIndex).first;
    cGate *downlinkI = sat->getOutputGate(isldirection::Direction::ISL_DOWNLINK, connection.satGateIndex).first;
    updateOrCreateChannel(uplinkO, downlinkI, delay, groundlinkDatarate);
    updateOrCreateChannel(downlinkO, uplinkI, delay, groundlinkDatarate);
}

void DtnTopologyControl::updateInterSatelliteLinks() {
    // if inter-plane ISL is not enabled/available
    if (interPlaneIslDisabled) return;

    switch (walkerType) {
        case WalkerType::DELTA:
            updateISLInWalkerDelta();
            break;
        case WalkerType::STAR:
            updateISLInWalkerStar();
            break;
        default:
            error("Error in TopologyControl::updateInterSatelliteLinks(): Unexpected WalkerType '%s'.", to_string(walkerType).c_str());
    }
}

void DtnTopologyControl::updateISLInWalkerDelta() {
    for (size_t index = 0; index < numSatellites; index++) {
        SatelliteRoutingBase *curSat = satellites.at(index);
        ASSERT(curSat != nullptr);

        // sat (o, i) = i-th sat in plane o
        // F = interplanefacing angle
        int satPlane = curSat->getPlane();
        SatelliteRoutingBase *rightSat = (satPlane != planeCount - 1)
                                             ? findSatByPlaneAndNumberInPlane(satPlane + 1, curSat->getNumberInPlane())
                                             : findSatByPlaneAndNumberInPlane(0, (curSat->getNumberInPlane() + interPlaneSpacing) % satsPerPlane);
        ASSERT(rightSat != nullptr);

        if (curSat->isAscending()) {
            // if next plane partner is descending, connection is not possible
            if (rightSat->isDescending()) {
                // if we were connected to that satellite on right
                if (curSat->hasRightSat() && curSat->getRightSatId() == rightSat->getId()) {
                    disconnectSatellites(curSat, rightSat, isldirection::Direction::ISL_RIGHT);
                }
                // if we were connected to that satellite on left
                else if (curSat->hasLeftSat() && curSat->getLeftSatId() == rightSat->getId()) {
                    disconnectSatellites(curSat, rightSat, isldirection::Direction::ISL_LEFT);
                }
            } else {
                connectSatellites(curSat, rightSat, isldirection::Direction::ISL_RIGHT);
            }
        }
        // sat ist descending
        else {
            // if next plane partner is not descending, connection is not possible
            if (rightSat->isAscending()) {
                // if we were connected to that satellite on right
                if (curSat->hasLeftSat() && curSat->getLeftSatId() == rightSat->getId()) {
                    disconnectSatellites(curSat, rightSat, isldirection::Direction::ISL_LEFT);
                }
                // if we were connected to that satellite on right
                else if (curSat->hasRightSat() && curSat->getRightSatId() == rightSat->getId()) {
                    disconnectSatellites(curSat, rightSat, isldirection::Direction::ISL_RIGHT);
                }
            } else {
                connectSatellites(curSat, rightSat, isldirection::Direction::ISL_LEFT);
            }
        }
    }
}

void DtnTopologyControl::updateISLInWalkerStar() {
    for (size_t index = 0; index < numSatellites; index++) {
        SatelliteRoutingBase *curSat = satellites.at(index);
        ASSERT(curSat != nullptr);
        int satPlane = curSat->getPlane();

        bool isLastPlane = satPlane == planeCount - 1;

        // if last plane stop because we reached the seam
        if (isLastPlane)
            break;
        int rightIndex = (index + satsPerPlane) % numSatellites;
        SatelliteRoutingBase *nextPlaneSat = satellites.at(rightIndex);
        ASSERT(nextPlaneSat != nullptr);

        if (curSat->isAscending()) {                                                                                          // sat is moving up
            if (nextPlaneSat->isAscending() && curSat->isInterPlaneISLEnabled() && nextPlaneSat->isInterPlaneISLEnabled()) {  // they are allowed to connect
                connectSatellites(curSat, nextPlaneSat, isldirection::Direction::ISL_RIGHT);
            } else {  // they are not allowed to have an connection
                disconnectSatellites(curSat, nextPlaneSat, isldirection::Direction::ISL_RIGHT);
            }
        } else {                                                                                                               // sat is moving down
            if (nextPlaneSat->isDescending() && curSat->isInterPlaneISLEnabled() && nextPlaneSat->isInterPlaneISLEnabled()) {  // they are allowed to connect
                connectSatellites(curSat, nextPlaneSat, isldirection::Direction::ISL_LEFT);
            } else {  // they are not allowed to have an connection
                disconnectSatellites(curSat, nextPlaneSat, isldirection::Direction::ISL_LEFT);
            }
        }
    }
}

}  // namespace topologycontrol
}  // namespace flora
