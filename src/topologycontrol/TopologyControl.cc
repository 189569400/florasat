/*
 * TopologyControl.cc
 *
 * Created on: Dec 20, 2022
 *     Author: Robin Ohs
 */

#include "TopologyControl.h"

namespace flora {
namespace topologycontrol {

Define_Module(TopologyControl);

TopologyControl::TopologyControl() : updateTimer(nullptr),
                                     isClosedConstellation(false),
                                     updateIntervalParameter(0),
                                     islDatarate(0.0),
                                     islDelay(0.0),
                                     minimumElevation(10.0),
                                     walkerType(WalkerType::UNINITIALIZED) {
}

TopologyControl::~TopologyControl() {
    cancelAndDeleteClockEvent(updateTimer);
}

void TopologyControl::initialize(int stage) {
    if (stage == inet::INITSTAGE_LOCAL) {
        updateIntervalParameter = &par("updateInterval");
        updateTimer = new ClockEvent("UpdateTimer");

        walkerType = WalkerType::parseWalkerType(par("walkerType"));
        isClosedConstellation = par("isClosedConstellation");
        lowerLatitudeBound = par("lowerLatitudeBound");
        upperLatitudeBound = par("upperLatitudeBound");
        islDelay = par("islDelay");
        islDatarate = par("islDatarate");
        groundlinkDelay = par("groundlinkDelay");
        groundlinkDatarate = par("groundlinkDatarate");
        minimumElevation = par("minimumElevation");
        isDtn = par("isDtn");
        EV << "Loaded parameters: "
           << "updateInterval: " << updateIntervalParameter << "; "
           << "isClosedConstellation: " << isClosedConstellation << "; "
           << "lowerLatitudeBound: " << lowerLatitudeBound << "; "
           << "upperLatitudeBound: " << upperLatitudeBound << "; "
           << "islDelay: " << islDelay << "; "
           << "islDatarate: " << islDatarate << "; "
           << "minimumElevation: " << minimumElevation << endl;
    } else if (stage == inet::INITSTAGE_APPLICATION_LAYER) {
        EV << "initialize TopologyControl" << endl;
        EV << "isDtn: " << isDtn << endl;

        loadSatellites();
        loadGroundstations();

        // take first satellite and read number of planes + satellitesPerPlane
        planeCount = satelliteInfos.at(0).noradModule->getNumberOfPlanes();
        satsPerPlane = satelliteInfos.at(0).noradModule->getSatellitesPerPlane();

        UpdateTopology();

        if (!updateTimer->isScheduled()) {
            scheduleUpdate();
        }
    }
}

void TopologyControl::handleMessage(cMessage *msg) {
    if (msg == updateTimer) {
        UpdateTopology();
        scheduleUpdate();
    } else
        throw cRuntimeError("TopologyControl: Unknown message.");
}

void TopologyControl::scheduleUpdate() {
    scheduleClockEventAfter(updateIntervalParameter->doubleValue(), updateTimer);
}

void TopologyControl::UpdateTopology() {
    if (satelliteInfos.size() == 0) {
        error("Error in TopologyControl::UpdateTopology(): No satellites found.");
        return;
    }
    // update ISL links and groundlinks
    topologyChanged = false;
    if (isDtn == false) {
        updateIntraSatelliteLinks();
        updateInterSatelliteLinks();
        updateGroundstationLinks();
    } else {
        updateGroundstationLinksDtn();
    }
    // if there was any change to the topology, track current contacts
    if (topologyChanged)
        trackTopologyChange();
}

GroundstationInfo *TopologyControl::getGroundstationInfo(int gsId) {
    if (gsId < 0 || gsId >= groundstationCount) {
        error("Error in TopologyControl::getGroundStationInfo: '%d' must be in range [0, %d)", gsId, groundstationCount);
    }
    return &groundstationInfos.at(gsId);
}

GsSatConnection *TopologyControl::getGroundstationSatConnection(int gsId, int satId) {
    return &gsSatConnections.at(std::pair<int, int>(gsId, satId));
}

void TopologyControl::loadGroundstations() {
    groundstationInfos.clear();
    groundstationCount = getSystemModule()->getSubmoduleVectorSize("groundStation");
    for (size_t i = 0; i < groundstationCount; i++) {
        cModule *groundstation = getSystemModule()->getSubmodule("groundStation", i);
        if (groundstation == nullptr) {
            error("Error in TopologyControl::getGroundstations(): groundStation with index %zu is nullptr. Make sure the module exists.", i);
        }
        GroundStationMobility *mobility = check_and_cast<GroundStationMobility *>(groundstation->getSubmodule("mobility"));
        if (mobility == nullptr) {
            error("Error in TopologyControl::getGroundstations(): mobility module of Groundstation is nullptr. Make sure a module with name `mobility` exists.");
        }
        GroundstationInfo created = GroundstationInfo(groundstation->par("groundStationId"), groundstation, mobility);
        groundstationInfos.emplace(i, created);
        EV << "Created GroundstationInfo" << created.to_string() << endl;
    }
}

void TopologyControl::loadSatellites() {
    satelliteInfos.clear();
    satelliteCount = getParentModule()->getSubmoduleVectorSize("loRaGW");
    for (size_t i = 0; i < satelliteCount; i++) {
        cModule *sat = getParentModule()->getSubmodule("loRaGW", i);
        if (sat == nullptr) {
            error("Error in TopologyControl::getSatellites(): loRaGW with index %zu is nullptr. Make sure the module exists.", i);
        }
        NoradA *noradA = check_and_cast<NoradA *>(sat->getSubmodule("NoradModule"));
        if (noradA == nullptr) {
            error("Error in TopologyControl::getSatellites(): noradA module of loRaGW with index %zu is nullptr. Make sure a module with name `NoradModule` exists.", i);
        }
        SatelliteInfo created = SatelliteInfo(i, sat, noradA);
        satelliteInfos.emplace(i, created);
    }
}

void TopologyControl::updateIntraSatelliteLinks() {
    // iterate over planes
    for (size_t plane = 0; plane < planeCount; plane++) {
        // iterate over sats in plane
        for (size_t planeSat = 0; planeSat < satsPerPlane; planeSat++) {
            int index = planeSat + plane * satsPerPlane;
            int isLastSatInPlane = index % satsPerPlane == satsPerPlane - 1;

            // if its only a "slice" of a constellation, no connection between the first and the last sat
            if (isLastSatInPlane && !isClosedConstellation) {
                EV << "Should continue?" << (isLastSatInPlane && !isClosedConstellation) << endl;
                continue;
            }

            // get the two satellites we want to connect. If we have the last in plane, we connect it to the first of the plane
            SatelliteInfo *curSat = &satelliteInfos.at(index);
            int nextId = isLastSatInPlane ? plane * satsPerPlane : index + 1;
            SatelliteInfo *otherSat = &satelliteInfos.at(nextId);

            // get gates
            cGate *fromGateIn = curSat->satelliteModule->gateHalf(ISL_UP_NAME.c_str(), cGate::Type::INPUT);
            cGate *fromGateOut = curSat->satelliteModule->gateHalf(ISL_UP_NAME.c_str(), cGate::Type::OUTPUT);
            cGate *toGateIn = otherSat->satelliteModule->gateHalf(ISL_DOWN_NAME.c_str(), cGate::Type::INPUT);
            cGate *toGateOut = otherSat->satelliteModule->gateHalf(ISL_DOWN_NAME.c_str(), cGate::Type::OUTPUT);

            // calculate ISL channel params
            double distance = curSat->noradModule->getDistance(otherSat->noradModule->getLatitude(), otherSat->noradModule->getLongitude(), otherSat->noradModule->getAltitude());
            double delay = islDelay * distance;

            // generate or update ISL channel from lower to upper sat
            ChannelState state1 = updateOrCreateChannel(fromGateOut, toGateIn, delay, islDatarate);
            ChannelState state2 = updateOrCreateChannel(toGateOut, fromGateIn, delay, islDatarate);

            // if any channel was created, we have a topologyupdate
            if (state1 == ChannelState::CREATED || state2 == ChannelState::CREATED) {
                topologyChanged = true;
                // Assign satellites in SatelliteInfo.
                // Important: Assumption that intra satellite links are not gonna change (in normal operation) after initial connect.
                curSat->upSatellite = nextId;
                otherSat->downSatellite = index;
            }
        }
    }
}

/**
 * Updates the links between each Groundstation and Satellites based on a ContactPlan instance.
 */
void TopologyControl::updateGroundstationLinksDtn() {
    ContactPlan *contactPlan = check_and_cast<ContactPlan *>(getParentModule()->getSubmodule("contactPlan"));
    if (contactPlan == nullptr) {
        error("Error in TopologyControl::updateNodeLinksDtn(): contactPlan is nullptr. Make sure the module exists.");
    }
    // TODO: Improve time complexity in this function from O(n^3)
    for (size_t i = 0; i < groundstationCount; i++) {
        GroundstationInfo *gsInfo = &groundstationInfos.at(i);
        std::set<SatelliteInfo *> satellites;
        for (size_t i = 0; i < satelliteCount; i++) {
            SatelliteInfo *satInfo = &satelliteInfos.at(i);
            vector<Contact> satContacts = contactPlan->getContactsBySrcDst(gsInfo->groundStationId, satInfo->satelliteId + groundstationCount);
            for (size_t i = 0; i < satContacts.size(); i++) {
                Contact contact = satContacts.at(i);
                if (isDtnContactStarting(gsInfo, satInfo, contact)){
                    linkGroundStationToSatDtn(gsInfo, satInfo);
                } else if (isDtnContactTakingPlace(gsInfo, satInfo, contact)) {
                    updateLinkGroundStationToSatDtn(gsInfo, satInfo);
                } else if (isDtnContactEnding(gsInfo, satInfo, contact)) {
                    unlinkGroundStationToSatDtn(gsInfo, satInfo);
                }
            }
        }
        EV << gsInfo->to_string() << endl;
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
bool TopologyControl::isDtnContactStarting(GroundstationInfo *gsInfo, SatelliteInfo *satInfo, Contact contact) {
    return contact.getSourceEid() == gsInfo->groundStationId && contact.getDestinationEid() == satInfo->satelliteId + groundstationCount && contact.getStart() == simTime().dbl();
}

/**
 * Checks if a contact between a GroundStation and Satellite is taking place based on a Contact instance
 *
 * @param gsInfo contains data related to a GroundStation
 * @param satInfo contains data related to a Satellite
 * @param contact represent a Contact between two nodes in a Contact Plan
 * @return whether a contact between a GroundStation and a Satellite is taking place
 */
bool TopologyControl::isDtnContactTakingPlace(GroundstationInfo *gsInfo, SatelliteInfo *satInfo, Contact contact) {
    return contact.getSourceEid() == gsInfo->groundStationId && contact.getDestinationEid() == satInfo->satelliteId + groundstationCount && contact.getEnd() > simTime().dbl() && contact.getStart() < simTime().dbl();
}

/**
 * Checks if a contact between a GroundStation and Satellite is ending based on a Contact instance
 *
 * @param gsInfo contains data related to a GroundStation
 * @param satInfo contains data related to a Satellite
 * @param contact represent a Contact between two nodes in a Contact Plan
 * @return whether a contact between a GroundStation and a Satellite is ending
 */
bool TopologyControl::isDtnContactEnding(GroundstationInfo *gsInfo, SatelliteInfo *satInfo, Contact contact) {
    return contact.getSourceEid() == gsInfo->groundStationId && contact.getDestinationEid() == satInfo->satelliteId + groundstationCount && contact.getEnd() == simTime().dbl();
}

/**
 * Creates a link between a GroundStation and Satellite
 * @param gsInfo contains data related to a GroundStation
 * @param satInfo contains data related to a Satellite
 */
void TopologyControl::linkGroundStationToSatDtn(GroundstationInfo *gsInfo, SatelliteInfo *satInfo) {
    double distance = satInfo->noradModule->getDistance(gsInfo->mobility->getLUTPositionY(), gsInfo->mobility->getLUTPositionX(), 0);
    double delay = distance * groundlinkDelay; // delay of the channel between satellite and groundstation (ms)
    EV << "Create channel between GS " << gsInfo->groundStationId << " and SAT " << satInfo->satelliteId << endl;
    int freeIndexGs = -1;
    for (size_t i = 0; i < 20; i++) {
        cGate *gate = gsInfo->groundStation->gateHalf(GS_SATLINK_NAME.c_str(), cGate::Type::OUTPUT, i);
        if (!gate->isConnectedOutside()) {
            freeIndexGs = i;
            break;
        }
    }
    if (freeIndexGs == -1) {
        error("No free gs gate index found.");
    }

    int freeIndexSat = -1;
    for (size_t i = 0; i < 20; i++) {
        cGate *gate = satInfo->satelliteModule->gateHalf(SAT_GROUNDLINK_NAME.c_str(), cGate::Type::OUTPUT, i);
        if (!gate->isConnectedOutside()) {
            freeIndexSat = i;
            break;
        }
    }
    if (freeIndexSat == -1) {
        error("No free sat gate index found.");
    }

    cGate *uplinkO = gsInfo->groundStation->gateHalf(GS_SATLINK_NAME.c_str(), cGate::Type::OUTPUT, freeIndexGs);
    cGate *uplinkI = gsInfo->groundStation->gateHalf(GS_SATLINK_NAME.c_str(), cGate::Type::INPUT, freeIndexGs);
    cGate *downlinkO = satInfo->satelliteModule->gateHalf(SAT_GROUNDLINK_NAME.c_str(), cGate::Type::OUTPUT, freeIndexSat);
    cGate *downlinkI = satInfo->satelliteModule->gateHalf(SAT_GROUNDLINK_NAME.c_str(), cGate::Type::INPUT, freeIndexSat);
    updateOrCreateChannel(uplinkO, downlinkI, delay, groundlinkDatarate);
    updateOrCreateChannel(downlinkO, uplinkI, delay, groundlinkDatarate);
    GsSatConnection connection = GsSatConnection(gsInfo, satInfo, freeIndexGs, freeIndexSat);
    gsSatConnections.emplace(std::pair<int, int>(gsInfo->groundStationId, satInfo->satelliteId), connection);
    topologyChanged = true;
    gsInfo->satellites.emplace(satInfo->satelliteId);
}

/**
 * Deletes the link between a GroundStation and Satellite
 * @param gsInfo contains data related to a GroundStation
 * @param satInfo contains data related to a Satellite
 */
void TopologyControl::unlinkGroundStationToSatDtn(GroundstationInfo *gsInfo, SatelliteInfo *satInfo) {
    GsSatConnection *connection = &gsSatConnections.at(std::pair<int, int>(gsInfo->groundStationId, satInfo->satelliteId));
    cGate *uplink = gsInfo->groundStation->gateHalf(GS_SATLINK_NAME.c_str(), cGate::Type::OUTPUT, connection->gsGateIndex);
    cGate *downlink = connection->satInfo->satelliteModule->gateHalf(SAT_GROUNDLINK_NAME.c_str(), cGate::Type::OUTPUT, connection->satGateIndex);
    deleteChannel(uplink);
    deleteChannel(downlink);
    gsSatConnections.erase(std::pair<int, int>(gsInfo->groundStationId, satInfo->satelliteId));
    topologyChanged = true;
    gsInfo->satellites.erase(satInfo->satelliteId);
}

/**
 * Update the link between a GroundStation and Satellite
 * @param gsInfo contains data related to a GroundStation
 * @param satInfo contains data related to a Satellite
 */
void TopologyControl::updateLinkGroundStationToSatDtn(GroundstationInfo *gsInfo, SatelliteInfo *satInfo) {
    double distance = satInfo->noradModule->getDistance(gsInfo->mobility->getLUTPositionY(), gsInfo->mobility->getLUTPositionX(), 0);
    double delay = distance * groundlinkDelay; // delay of the channel between nearest satellite and groundstation (ms)
    GsSatConnection *connection = &gsSatConnections.at(std::pair<int, int>(gsInfo->groundStationId, satInfo->satelliteId));
    cGate *uplinkO = gsInfo->groundStation->gateHalf(GS_SATLINK_NAME.c_str(), cGate::Type::OUTPUT, connection->gsGateIndex);
    cGate *uplinkI = gsInfo->groundStation->gateHalf(GS_SATLINK_NAME.c_str(), cGate::Type::INPUT, connection->gsGateIndex);
    cGate *downlinkO = satInfo->satelliteModule->gateHalf(SAT_GROUNDLINK_NAME.c_str(), cGate::Type::OUTPUT, connection->satGateIndex);
    cGate *downlinkI = satInfo->satelliteModule->gateHalf(SAT_GROUNDLINK_NAME.c_str(), cGate::Type::INPUT, connection->satGateIndex);
    updateOrCreateChannel(uplinkO, downlinkI, delay, groundlinkDatarate);
    updateOrCreateChannel(downlinkO, uplinkI, delay, groundlinkDatarate);
}

void TopologyControl::updateGroundstationLinks() {
    // iterate over groundstations
    for (size_t i = 0; i < groundstationCount; i++) {
        GroundstationInfo *gsInfo = &groundstationInfos.at(i);
        EV << gsInfo->to_string() << endl;

        // find satellites with elevation >= minElevation
        std::set<SatelliteInfo *> satellites;
        for (size_t i = 0; i < satelliteCount; i++) {
            SatelliteInfo *satInfo = &satelliteInfos.at(i);
            double elevation = ((INorad *)satInfo->noradModule)->getElevation(gsInfo->mobility->getLUTPositionY(), gsInfo->mobility->getLUTPositionX(), 0);
            if (elevation >= minimumElevation) {
                satellites.emplace(satInfo);
            }
        }

        std::set<int> oldConnections = gsInfo->satellites;

        for (SatelliteInfo *satInfo : satellites) {
            double distance = satInfo->noradModule->getDistance(gsInfo->mobility->getLUTPositionY(), gsInfo->mobility->getLUTPositionX(), 0);
            // delay of the channel between nearest satellite and groundstation (ms)
            double delay = distance * groundlinkDelay;

            // connected before and remains -> update channel
            if (std::find(oldConnections.begin(), oldConnections.end(), satInfo->satelliteId) != oldConnections.end()) {
                EV << "Update channel between GS " << gsInfo->groundStationId << " and SAT " << satInfo->satelliteId << endl;

                GsSatConnection *connection = &gsSatConnections.at(std::pair<int, int>(gsInfo->groundStationId, satInfo->satelliteId));
                cGate *uplinkO = gsInfo->groundStation->gateHalf(GS_SATLINK_NAME.c_str(), cGate::Type::OUTPUT, connection->gsGateIndex);
                cGate *uplinkI = gsInfo->groundStation->gateHalf(GS_SATLINK_NAME.c_str(), cGate::Type::INPUT, connection->gsGateIndex);
                cGate *downlinkO = satInfo->satelliteModule->gateHalf(SAT_GROUNDLINK_NAME.c_str(), cGate::Type::OUTPUT, connection->satGateIndex);
                cGate *downlinkI = satInfo->satelliteModule->gateHalf(SAT_GROUNDLINK_NAME.c_str(), cGate::Type::INPUT, connection->satGateIndex);
                updateOrCreateChannel(uplinkO, downlinkI, delay, groundlinkDatarate);
                updateOrCreateChannel(downlinkO, uplinkI, delay, groundlinkDatarate);
            } else {
                EV << "Create channel between GS " << gsInfo->groundStationId << " and SAT " << satInfo->satelliteId << endl;

                int freeIndexGs = -1;
                for (size_t i = 0; i < 20; i++) {
                    cGate *gate = gsInfo->groundStation->gateHalf(GS_SATLINK_NAME.c_str(), cGate::Type::OUTPUT, i);
                    if (!gate->isConnectedOutside()) {
                        freeIndexGs = i;
                        break;
                    }
                }
                if (freeIndexGs == -1) {
                    error("No free gs gate index found.");
                }

                int freeIndexSat = -1;
                for (size_t i = 0; i < 20; i++) {
                    cGate *gate = satInfo->satelliteModule->gateHalf(SAT_GROUNDLINK_NAME.c_str(), cGate::Type::OUTPUT, i);
                    if (!gate->isConnectedOutside()) {
                        freeIndexSat = i;
                        break;
                    }
                }
                if (freeIndexSat == -1) {
                    error("No free sat gate index found.");
                }

                cGate *uplinkO = gsInfo->groundStation->gateHalf(GS_SATLINK_NAME.c_str(), cGate::Type::OUTPUT, freeIndexGs);
                cGate *uplinkI = gsInfo->groundStation->gateHalf(GS_SATLINK_NAME.c_str(), cGate::Type::INPUT, freeIndexGs);
                cGate *downlinkO = satInfo->satelliteModule->gateHalf(SAT_GROUNDLINK_NAME.c_str(), cGate::Type::OUTPUT, freeIndexSat);
                cGate *downlinkI = satInfo->satelliteModule->gateHalf(SAT_GROUNDLINK_NAME.c_str(), cGate::Type::INPUT, freeIndexSat);
                updateOrCreateChannel(uplinkO, downlinkI, delay, groundlinkDatarate);
                updateOrCreateChannel(downlinkO, uplinkI, delay, groundlinkDatarate);
                GsSatConnection connection = GsSatConnection(gsInfo, satInfo, freeIndexGs, freeIndexSat);
                gsSatConnections.emplace(std::pair<int, int>(gsInfo->groundStationId, satInfo->satelliteId), connection);
                topologyChanged = true;
            }
            oldConnections.erase(satInfo->satelliteId);
        }

        for (int removeConnectionSatId : oldConnections) {
            EV << "Delete channel between GS " << gsInfo->groundStationId << " and SAT " << removeConnectionSatId << endl;

            GsSatConnection *connection = &gsSatConnections.at(std::pair<int, int>(gsInfo->groundStationId, removeConnectionSatId));
            cGate *uplink = gsInfo->groundStation->gateHalf(GS_SATLINK_NAME.c_str(), cGate::Type::OUTPUT, connection->gsGateIndex);
            cGate *downlink = connection->satInfo->satelliteModule->gateHalf(SAT_GROUNDLINK_NAME.c_str(), cGate::Type::OUTPUT, connection->satGateIndex);
            deleteChannel(uplink);
            deleteChannel(downlink);
            gsSatConnections.erase(std::pair<int, int>(gsInfo->groundStationId, removeConnectionSatId));
            topologyChanged = true;
        }

        gsInfo->satellites.clear();
        for (auto sat : satellites) {
            gsInfo->satellites.emplace(sat->satelliteId);
        }

        EV << gsInfo->to_string() << endl;
    }
}

void TopologyControl::updateInterSatelliteLinks() {
    switch (walkerType) {
        case WalkerType::DELTA:
            updateISLInWalkerDelta();
            break;
        case WalkerType::STAR:
            updateISLInWalkerStar();
            break;
        default:
            error("Error in TopologyControl::updateInterSatelliteLinks(): Unexpected WalkerType '%s'.", WalkerType::as_string(walkerType).c_str());
    }
}

void TopologyControl::updateISLInWalkerDelta() {
    for (size_t index = 0; index < satelliteCount; index++) {
        SatelliteInfo *curSat = &satelliteInfos.at(index);
        int satPlane = calculateSatellitePlane(index);
        int nextPlane = (satPlane + 1) % planeCount;

        int nextPlaneSatIndex = (index + satsPerPlane) % satelliteCount;
        SatelliteInfo *expectedRightSat = &satelliteInfos.at(nextPlaneSatIndex);

        if (curSat->noradModule->isAscending()) {
            cGate *rightGateOut = curSat->satelliteModule->gateHalf(ISL_RIGHT_NAME.c_str(), cGate::Type::OUTPUT);
            cGate *leftGateOut = expectedRightSat->satelliteModule->gateHalf(ISL_LEFT_NAME.c_str(), cGate::Type::OUTPUT);

            // if next plane partner is not ascending, connection is not possible
            if (!expectedRightSat->noradModule->isAscending()) {
                // if we were connected to that satellite on right
                if (curSat->rightSatellite == nextPlaneSatIndex) {
                    deleteChannel(rightGateOut);
                    deleteChannel(leftGateOut);
                    curSat->rightSatellite = -1;
                    expectedRightSat->leftSatellite = -1;
                    topologyChanged = true;
                }
                // if we were connected to that satellite on left
                else if (curSat->leftSatellite == nextPlaneSatIndex) {
                    cGate *leftGateOutOther = curSat->satelliteModule->gateHalf(ISL_LEFT_NAME.c_str(), cGate::Type::OUTPUT);
                    cGate *rightGateOutOther = expectedRightSat->satelliteModule->gateHalf(ISL_RIGHT_NAME.c_str(), cGate::Type::OUTPUT);
                    deleteChannel(leftGateOutOther);
                    deleteChannel(rightGateOutOther);
                    curSat->leftSatellite = -1;
                    expectedRightSat->rightSatellite = -1;
                    topologyChanged = true;
                }
            } else {
                cGate *rightGateIn = curSat->satelliteModule->gateHalf(ISL_RIGHT_NAME.c_str(), cGate::Type::INPUT);
                cGate *leftGateIn = expectedRightSat->satelliteModule->gateHalf(ISL_LEFT_NAME.c_str(), cGate::Type::INPUT);
                // disconnect the other satellite if has left satellite and it is not ours
                if (expectedRightSat->leftSatellite != index && expectedRightSat->leftSatellite != -1) {
                    SatelliteInfo *otherSatOldPartner = &satelliteInfos.at(expectedRightSat->leftSatellite);
                    cGate *rightGateOutOld = otherSatOldPartner->satelliteModule->gateHalf(ISL_RIGHT_NAME.c_str(), cGate::Type::OUTPUT);
                    deleteChannel(leftGateOut);
                    deleteChannel(rightGateOutOld);
                    expectedRightSat->leftSatellite = -1;
                    otherSatOldPartner->rightSatellite = -1;
                    topologyChanged = true;
                }

                double distance = curSat->noradModule->getDistance(expectedRightSat->noradModule->getLatitude(), expectedRightSat->noradModule->getLongitude(), expectedRightSat->noradModule->getAltitude());
                double delay = islDelay * distance;

                // generate or update ISL channel from right to left sat
                ChannelState state1 = updateOrCreateChannel(rightGateOut, leftGateIn, delay, islDatarate);
                ChannelState state2 = updateOrCreateChannel(leftGateOut, rightGateIn, delay, islDatarate);

                // if any channel was created, we have a topologyupdate
                if (state1 == ChannelState::CREATED || state2 == ChannelState::CREATED) {
                    curSat->rightSatellite = nextPlaneSatIndex;
                    expectedRightSat->leftSatellite = index;
                    topologyChanged = true;
                }
            }
        }
        // sat ist descending
        else {
            cGate *leftGateOut = curSat->satelliteModule->gateHalf(ISL_LEFT_NAME.c_str(), cGate::Type::OUTPUT);
            cGate *rightGateOut = expectedRightSat->satelliteModule->gateHalf(ISL_RIGHT_NAME.c_str(), cGate::Type::OUTPUT);

            // if next plane partner is not descending, connection is not possible
            if (expectedRightSat->noradModule->isAscending()) {
                // if we were connected to that satellite on right
                if (curSat->leftSatellite == nextPlaneSatIndex) {
                    deleteChannel(leftGateOut);
                    deleteChannel(rightGateOut);
                    curSat->leftSatellite = -1;
                    expectedRightSat->rightSatellite = -1;
                    topologyChanged = true;
                }
                // if we were connected to that satellite on right
                else if (curSat->rightSatellite == nextPlaneSatIndex) {
                    cGate *rightGateOutOther = curSat->satelliteModule->gateHalf(ISL_RIGHT_NAME.c_str(), cGate::Type::OUTPUT);
                    cGate *leftGateOutOther = expectedRightSat->satelliteModule->gateHalf(ISL_LEFT_NAME.c_str(), cGate::Type::OUTPUT);
                    deleteChannel(rightGateOutOther);
                    deleteChannel(leftGateOutOther);
                    curSat->rightSatellite = -1;
                    expectedRightSat->leftSatellite = -1;
                    topologyChanged = true;
                }
            } else {
                cGate *leftGateIn = curSat->satelliteModule->gateHalf(ISL_LEFT_NAME.c_str(), cGate::Type::INPUT);
                cGate *rightGateIn = expectedRightSat->satelliteModule->gateHalf(ISL_RIGHT_NAME.c_str(), cGate::Type::INPUT);

                // disconnect the other satellite if has right satellite and it is not ours
                if (expectedRightSat->rightSatellite != index && expectedRightSat->rightSatellite != -1) {
                    SatelliteInfo *otherSatOldPartner = &satelliteInfos.at(expectedRightSat->rightSatellite);
                    cGate *leftGateOutOld = otherSatOldPartner->satelliteModule->gateHalf(ISL_LEFT_NAME.c_str(), cGate::Type::OUTPUT);
                    deleteChannel(rightGateOut);
                    deleteChannel(leftGateOutOld);
                    expectedRightSat->rightSatellite = -1;
                    otherSatOldPartner->leftSatellite = -1;
                    topologyChanged = true;
                }

                double distance = curSat->noradModule->getDistance(expectedRightSat->noradModule->getLatitude(), expectedRightSat->noradModule->getLongitude(), expectedRightSat->noradModule->getAltitude());
                double delay = islDelay * distance;

                // generate or update ISL channel from right to left sat
                ChannelState state1 = updateOrCreateChannel(rightGateOut, leftGateIn, delay, islDatarate);
                ChannelState state2 = updateOrCreateChannel(leftGateOut, rightGateIn, delay, islDatarate);

                // if any channel was created, we have a topologyupdate
                if (state1 == ChannelState::CREATED || state2 == ChannelState::CREATED) {
                    curSat->leftSatellite = nextPlaneSatIndex;
                    expectedRightSat->rightSatellite = index;
                    topologyChanged = true;
                }
            }
        }
    }
}

void TopologyControl::updateISLInWalkerStar() {
    for (size_t index = 0; index < satelliteCount; index++) {
        SatelliteInfo *curSat = &satelliteInfos.at(index);
        int satPlane = calculateSatellitePlane(index);

        bool isLastPlane = satPlane == planeCount - 1;

        // if is last plane stop because it is the seam
        if (isLastPlane)
            break;
        int rightIndex = (index + satsPerPlane) % satelliteCount;
        SatelliteInfo *nextPlaneSat = &satelliteInfos.at(rightIndex);

        // calculate ISL channel params
        double distance = curSat->noradModule->getDistance(nextPlaneSat->noradModule->getLatitude(), nextPlaneSat->noradModule->getLongitude(), nextPlaneSat->noradModule->getAltitude());
        double delay = islDelay * distance;

        if (curSat->noradModule->isAscending()) {  // sat is moving up
            cGate *rightGateOut = curSat->satelliteModule->gateHalf(ISL_RIGHT_NAME.c_str(), cGate::Type::OUTPUT);
            cGate *leftGateOut = nextPlaneSat->satelliteModule->gateHalf(ISL_LEFT_NAME.c_str(), cGate::Type::OUTPUT);

            if (nextPlaneSat->noradModule->isAscending() && isIslEnabled(curSat->noradModule->getLatitude()) && isIslEnabled(nextPlaneSat->noradModule->getLatitude())) {  // they are allowed to connect
                cGate *rightGateIn = curSat->satelliteModule->gateHalf(ISL_RIGHT_NAME.c_str(), cGate::Type::INPUT);
                cGate *leftGateIn = nextPlaneSat->satelliteModule->gateHalf(ISL_LEFT_NAME.c_str(), cGate::Type::INPUT);

                // generate or update ISL channel from right to left sat
                ChannelState state1 = updateOrCreateChannel(rightGateOut, leftGateIn, delay, islDatarate);
                ChannelState state2 = updateOrCreateChannel(leftGateOut, rightGateIn, delay, islDatarate);

                // if any channel was created, we have a topologyupdate
                if (state1 == ChannelState::CREATED || state2 == ChannelState::CREATED) {
                    curSat->rightSatellite = rightIndex;
                    nextPlaneSat->leftSatellite = index;
                    topologyChanged = true;
                }
            } else {  // they are not allowed to have an connection
                ChannelState state1 = deleteChannel(rightGateOut);
                ChannelState state2 = deleteChannel(leftGateOut);

                // if any channel was deleted, we have a topologyupdate
                if (state1 == ChannelState::DELETED || state2 == ChannelState::DELETED) {
                    curSat->rightSatellite = -1;
                    nextPlaneSat->leftSatellite = -1;
                    topologyChanged = true;
                }
            }
        } else {  // sat is moving down
            cGate *leftGateOut = curSat->satelliteModule->gateHalf(ISL_LEFT_NAME.c_str(), cGate::Type::OUTPUT);
            cGate *rightGateOut = nextPlaneSat->satelliteModule->gateHalf(ISL_RIGHT_NAME.c_str(), cGate::Type::OUTPUT);

            if (!nextPlaneSat->noradModule->isAscending() && isIslEnabled(curSat->noradModule->getLatitude()) && isIslEnabled(nextPlaneSat->noradModule->getLatitude())) {  // they are allowed to connect
                cGate *leftGateIn = curSat->satelliteModule->gateHalf(ISL_LEFT_NAME.c_str(), cGate::Type::INPUT);
                cGate *rightGateIn = nextPlaneSat->satelliteModule->gateHalf(ISL_RIGHT_NAME.c_str(), cGate::Type::INPUT);

                // generate or update ISL channel from right to left sat
                ChannelState state1 = updateOrCreateChannel(leftGateOut, rightGateIn, delay, islDatarate);
                ChannelState state2 = updateOrCreateChannel(rightGateOut, leftGateIn, delay, islDatarate);

                // if any channel was created, we have a topologyupdate
                if (state1 == ChannelState::CREATED || state2 == ChannelState::CREATED) {
                    curSat->leftSatellite = rightIndex;
                    nextPlaneSat->rightSatellite = index;
                    topologyChanged = true;
                }
            } else {  // they are not allowed to have an connection
                ChannelState state1 = deleteChannel(leftGateOut);
                ChannelState state2 = deleteChannel(rightGateOut);

                // if any channel was deleted, we have a topologyupdate
                if (state1 == ChannelState::DELETED || state2 == ChannelState::DELETED) {
                    curSat->leftSatellite = -1;
                    nextPlaneSat->rightSatellite = -1;
                    topologyChanged = true;
                }
            }
        }
    }
}

bool TopologyControl::isIslEnabled(double latitude) {
    return latitude <= upperLatitudeBound && latitude >= lowerLatitudeBound;
}

ChannelState TopologyControl::updateOrCreateChannel(cGate *outGate, cGate *inGate, double delay, double datarate) {
    if (outGate->isConnectedOutside()) {
        cDatarateChannel *channel = check_and_cast<cDatarateChannel *>(outGate->getChannel());
        channel->setDelay(delay);
        channel->setDatarate(datarate);
        return ChannelState::UPDATED;
    } else {
        cDatarateChannel *channel = cDatarateChannel::create(ISL_CHANNEL_NAME.c_str());
        channel->setDelay(delay);
        channel->setDatarate(datarate);
        outGate->connectTo(inGate, channel);
        channel->callInitialize();
        return ChannelState::CREATED;
    }
}

ChannelState TopologyControl::deleteChannel(cGate *outGate) {
    if (outGate->isConnectedOutside()) {
        outGate->disconnect();
        return ChannelState::DELETED;
    }
    return ChannelState::UNCHANGED;
}

int TopologyControl::calculateSatellitePlane(int id) {
    int satPlane = (id - (id % satsPerPlane)) / satsPerPlane;
    if (satPlane > planeCount - 1) {
        error("Error in TopologyControl::calculateSatellitePlane(): Calculated plane (%d) is bigger than number of planes - 1 (%d).", satPlane, planeCount - 1);
    }
    return satPlane;
}

void TopologyControl::trackTopologyChange() {
    EV << "Topology was changed at " << simTime() << endl;
    for (size_t i = 0; i < satelliteCount; i++) {
        SatelliteInfo *satInfo = &satelliteInfos.at(i);
        EV << satInfo->to_string() << endl;
    }
}

}  // namespace topologycontrol
}  // namespace flora
