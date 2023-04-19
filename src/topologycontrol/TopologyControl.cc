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
        lowerLatitudeBound = par("lowerLatitudeBound");
        upperLatitudeBound = par("upperLatitudeBound");
        islDelay = par("islDelay");
        islDatarate = par("islDatarate");
        groundlinkDelay = par("groundlinkDelay");
        groundlinkDatarate = par("groundlinkDatarate");
        numGroundLinks = par("numGroundLinks");
        minimumElevation = par("minimumElevation");
        EV << "Loaded parameters: "
           << "updateInterval: " << updateIntervalParameter << "; "
           << "lowerLatitudeBound: " << lowerLatitudeBound << "; "
           << "upperLatitudeBound: " << upperLatitudeBound << "; "
           << "islDelay: " << islDelay << "; "
           << "islDatarate: " << islDatarate << "; "
           << "numGroundLinks: " << numGroundLinks << "; "
           << "minimumElevation: " << minimumElevation << endl;
    } else if (stage == inet::INITSTAGE_PHYSICAL_LAYER) {
        EV << "Initialize TopologyControl" << endl;

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
        error("TopologyControl: Unknown message.");
}

void TopologyControl::scheduleUpdate() {
    scheduleClockEventAfter(updateIntervalParameter->doubleValue(), updateTimer);
}

void TopologyControl::UpdateTopology() {
    if (satelliteInfos.size() == 0) {
        error("Error in TopologyControl::UpdateTopology(): No satellites found.");
        return;
    }
    core::Timer timer = core::Timer();
    // update ISL links and groundlinks
    topologyChanged = false;
    updateIntraSatelliteLinks();
    updateInterSatelliteLinks();
    updateGroundstationLinks();

    EV << "TC: Calculation took " << timer.getTime() / 1000 / 1000 << "ms" << endl;
    // if there was any change to the topology, track current contacts
    if (topologyChanged)
        trackTopologyChange();
}

GroundstationInfo *TopologyControl::getGroundstationInfo(int gsId) {
    if (gsId < 0 || gsId >= numGroundStations) {
        error("Error in TopologyControl::getGroundStationInfo: '%d' must be in range [0, %d)", gsId, numGroundStations);
    }
    return &groundstationInfos.at(gsId);
}

GsSatConnection *TopologyControl::getGroundstationSatConnection(int gsId, int satId) {
    return &gsSatConnections.at(std::pair<int, int>(gsId, satId));
}

void TopologyControl::loadGroundstations() {
    groundstationInfos.clear();
    numGroundStations = getSystemModule()->getSubmoduleVectorSize("groundStation");
    for (size_t i = 0; i < numGroundStations; i++) {
        cModule *groundstation = getSystemModule()->getSubmodule("groundStation", i);
        if (groundstation == nullptr) {
            error("Error in TopologyControl::getGroundstations(): groundStation with index %zu is nullptr. Make sure the module exists.", i);
        }
        GroundStationMobility *mobility = check_and_cast<GroundStationMobility *>(groundstation->getSubmodule("gsMobility"));
        if (mobility == nullptr) {
            error("Error in TopologyControl::getGroundstations(): mobility module of Groundstation is nullptr. Make sure a module with name `gsMobility` exists.");
        }
        GroundstationInfo created = GroundstationInfo(groundstation->par("groundStationId"), groundstation, mobility);
        groundstationInfos.emplace(i, created);
    }
}

void TopologyControl::loadSatellites() {
    satelliteInfos.clear();
    numSatellites = getSystemModule()->getSubmoduleVectorSize("loRaGW");
    for (size_t i = 0; i < numSatellites; i++) {
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

            // get the two satellites we want to connect. If we have the last in plane, we connect it to the first of the plane
            SatelliteInfo &curSat = satelliteInfos.at(index);
            int nextId = isLastSatInPlane ? plane * satsPerPlane : index + 1;
            SatelliteInfo &otherSat = satelliteInfos.at(nextId);

            // get gates
            cGate *fromGateIn = curSat.getInputGate(Direction::ISL_UP);
            cGate *fromGateOut = curSat.getOutputGate(Direction::ISL_UP);
            cGate *toGateIn = otherSat.getInputGate(Direction::ISL_DOWN);
            cGate *toGateOut = otherSat.getOutputGate(Direction::ISL_DOWN);

            // calculate ISL channel params
            double delay = islDelay * curSat.getDistance(otherSat);

            // generate or update ISL channel from lower to upper sat
            ChannelState state1 = updateOrCreateChannel(fromGateOut, toGateIn, delay, islDatarate);
            ChannelState state2 = updateOrCreateChannel(toGateOut, fromGateIn, delay, islDatarate);

            // if any channel was created, we have a topologyupdate
            if (state1 == ChannelState::CREATED || state2 == ChannelState::CREATED) {
                topologyChanged = true;
                // Assign satellites in SatelliteInfo.
                // Important: Assumption that intra satellite links are not gonna change (in normal operation) after initial connect.
                curSat.upSatellite = nextId;
                otherSat.downSatellite = index;
            }
        }
    }
}

void TopologyControl::updateGroundstationLinks() {
    // iterate over groundstations
    for (size_t gsId = 0; gsId < numGroundStations; gsId++) {
        GroundstationInfo &gsInfo = groundstationInfos.at(gsId);
        for (size_t satId = 0; satId < numSatellites; satId++) {
            SatelliteInfo &satInfo = satelliteInfos.at(satId);

            bool isOldConnection = utils::set::contains<int>(gsInfo.satellites, satId);

            // if is not in range continue with next sat
            if (satInfo.getElevation(gsInfo) < minimumElevation) {
                // if they were previous connected, delete that connection
                if (isOldConnection) {
                    GsSatConnection &connection = gsSatConnections.at(std::pair<int, int>(gsId, satId));
                    cGate *uplink = gsInfo.getOutputGate(connection.gsGateIndex);
                    cGate *downlink = satInfo.getOutputGate(Direction::ISL_DOWNLINK, connection.satGateIndex);
                    deleteChannel(uplink);
                    deleteChannel(downlink);
                    gsSatConnections.erase(std::pair<int, int>(gsId, satId));
                    gsInfo.satellites.erase(satId);
                    topologyChanged = true;
                }
                // in all cases continue with next sat
                continue;
            }

            double delay = satInfo.getDistance(gsInfo) * groundlinkDelay;

            // if they were previous connected, update channel with new delay
            if (isOldConnection) {
                GsSatConnection &connection = gsSatConnections.at(std::pair<int, int>(gsId, satId));
                cGate *uplinkO = gsInfo.getOutputGate(connection.gsGateIndex);
                cGate *uplinkI = gsInfo.getInputGate(connection.gsGateIndex);
                cGate *downlinkO = satInfo.getOutputGate(Direction::ISL_DOWNLINK, connection.satGateIndex);
                cGate *downlinkI = satInfo.getInputGate(Direction::ISL_DOWNLINK, connection.satGateIndex);
                updateOrCreateChannel(uplinkO, downlinkI, delay, groundlinkDatarate);
                updateOrCreateChannel(downlinkO, uplinkI, delay, groundlinkDatarate);
            }
            // they were not previous connected, create new channel between gs and sat
            else {
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
                    cGate *gate = satInfo.getOutputGate(Direction::ISL_DOWNLINK, i);
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
                cGate *downlinkO = satInfo.getOutputGate(Direction::ISL_DOWNLINK, freeIndexSat);
                cGate *downlinkI = satInfo.getInputGate(Direction::ISL_DOWNLINK, freeIndexSat);
                updateOrCreateChannel(uplinkO, downlinkI, delay, groundlinkDatarate);
                updateOrCreateChannel(downlinkO, uplinkI, delay, groundlinkDatarate);
                gsSatConnections.emplace(std::pair<int, int>(gsId, satId), GsSatConnection(gsId, satId, freeIndexGs, freeIndexSat));
                gsInfo.satellites.emplace(satId);
                topologyChanged = true;
            }
        }
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
    for (size_t index = 0; index < numSatellites; index++) {
        SatelliteInfo &curSat = satelliteInfos.at(index);
        int nextPlaneSatIndex = (index + satsPerPlane) % numSatellites;
        SatelliteInfo &expectedRightSat = satelliteInfos.at(nextPlaneSatIndex);

        if (curSat.isAscending()) {
            cGate *rightGateOut = curSat.getOutputGate(Direction::ISL_RIGHT);
            cGate *leftGateOut = expectedRightSat.getOutputGate(Direction::ISL_LEFT);

            // if next plane partner is descending, connection is not possible
            if (expectedRightSat.isDescending()) {
                // if we were connected to that satellite on right
                if (curSat.rightSatellite == nextPlaneSatIndex) {
                    deleteChannel(rightGateOut);
                    deleteChannel(leftGateOut);
                    curSat.rightSatellite = -1;
                    expectedRightSat.leftSatellite = -1;
                    topologyChanged = true;
                }
                // if we were connected to that satellite on left
                else if (curSat.leftSatellite == nextPlaneSatIndex) {
                    cGate *leftGateOutOther = curSat.getOutputGate(Direction::ISL_LEFT);
                    cGate *rightGateOutOther = expectedRightSat.getOutputGate(Direction::ISL_RIGHT);
                    deleteChannel(leftGateOutOther);
                    deleteChannel(rightGateOutOther);
                    curSat.leftSatellite = -1;
                    expectedRightSat.rightSatellite = -1;
                    topologyChanged = true;
                }
            } else {
                cGate *rightGateIn = curSat.getInputGate(Direction::ISL_RIGHT);
                cGate *leftGateIn = expectedRightSat.getInputGate(Direction::ISL_LEFT);
                // disconnect the other satellite if has left satellite and it is not ours
                if (expectedRightSat.leftSatellite != index && expectedRightSat.leftSatellite != -1) {
                    SatelliteInfo &otherSatOldPartner = satelliteInfos.at(expectedRightSat.leftSatellite);
                    cGate *rightGateOutOld = otherSatOldPartner.getOutputGate(Direction::ISL_RIGHT);
                    deleteChannel(leftGateOut);
                    deleteChannel(rightGateOutOld);
                    expectedRightSat.leftSatellite = -1;
                    otherSatOldPartner.rightSatellite = -1;
                    topologyChanged = true;
                }

                double distance = curSat.getDistance(expectedRightSat);
                double delay = islDelay * distance;

                // generate or update ISL channel from right to left sat
                ChannelState state1 = updateOrCreateChannel(rightGateOut, leftGateIn, delay, islDatarate);
                ChannelState state2 = updateOrCreateChannel(leftGateOut, rightGateIn, delay, islDatarate);

                // if any channel was created, we have a topologyupdate
                if (state1 == ChannelState::CREATED || state2 == ChannelState::CREATED) {
                    curSat.rightSatellite = nextPlaneSatIndex;
                    expectedRightSat.leftSatellite = index;
                    topologyChanged = true;
                }
            }
        }
        // sat ist descending
        else {
            cGate *leftGateOut = curSat.getOutputGate(Direction::ISL_LEFT);
            cGate *rightGateOut = expectedRightSat.getOutputGate(Direction::ISL_RIGHT);

            // if next plane partner is not descending, connection is not possible
            if (expectedRightSat.isAscending()) {
                // if we were connected to that satellite on right
                if (curSat.leftSatellite == nextPlaneSatIndex) {
                    deleteChannel(leftGateOut);
                    deleteChannel(rightGateOut);
                    curSat.leftSatellite = -1;
                    expectedRightSat.rightSatellite = -1;
                    topologyChanged = true;
                }
                // if we were connected to that satellite on right
                else if (curSat.rightSatellite == nextPlaneSatIndex) {
                    cGate *rightGateOutOther = curSat.getOutputGate(Direction::ISL_RIGHT);
                    cGate *leftGateOutOther = expectedRightSat.getOutputGate(Direction::ISL_LEFT);
                    deleteChannel(rightGateOutOther);
                    deleteChannel(leftGateOutOther);
                    curSat.rightSatellite = -1;
                    expectedRightSat.leftSatellite = -1;
                    topologyChanged = true;
                }
            } else {
                cGate *leftGateIn = curSat.getInputGate(Direction::ISL_LEFT);
                cGate *rightGateIn = expectedRightSat.getInputGate(Direction::ISL_RIGHT);

                // disconnect the other satellite if has right satellite and it is not ours
                if (expectedRightSat.rightSatellite != index && expectedRightSat.rightSatellite != -1) {
                    SatelliteInfo &otherSatOldPartner = satelliteInfos.at(expectedRightSat.rightSatellite);
                    cGate *leftGateOutOld = otherSatOldPartner.getOutputGate(Direction::ISL_LEFT);
                    deleteChannel(rightGateOut);
                    deleteChannel(leftGateOutOld);
                    expectedRightSat.rightSatellite = -1;
                    otherSatOldPartner.leftSatellite = -1;
                    topologyChanged = true;
                }

                double delay = islDelay * curSat.getDistance(expectedRightSat);

                // generate or update ISL channel from right to left sat
                ChannelState state1 = updateOrCreateChannel(rightGateOut, leftGateIn, delay, islDatarate);
                ChannelState state2 = updateOrCreateChannel(leftGateOut, rightGateIn, delay, islDatarate);

                // if any channel was created, we have a topologyupdate
                if (state1 == ChannelState::CREATED || state2 == ChannelState::CREATED) {
                    curSat.leftSatellite = nextPlaneSatIndex;
                    expectedRightSat.rightSatellite = index;
                    topologyChanged = true;
                }
            }
        }
    }
}

void TopologyControl::updateISLInWalkerStar() {
    for (size_t index = 0; index < numSatellites; index++) {
        SatelliteInfo &curSat = satelliteInfos.at(index);
        int satPlane = calculateSatellitePlane(index);

        bool isLastPlane = satPlane == planeCount - 1;

        // if is last plane stop because it is the seam
        if (isLastPlane)
            break;
        int rightIndex = (index + satsPerPlane) % numSatellites;
        SatelliteInfo &nextPlaneSat = satelliteInfos.at(rightIndex);

        // calculate ISL channel params
        double delay = islDelay * curSat.getDistance(nextPlaneSat);

        if (curSat.isAscending()) {  // sat is moving up
            cGate *rightGateOut = curSat.getOutputGate(Direction::ISL_RIGHT);
            cGate *leftGateOut = nextPlaneSat.getOutputGate(Direction::ISL_LEFT);

            if (nextPlaneSat.isAscending() && isIslEnabled(curSat) && isIslEnabled(nextPlaneSat)) {  // they are allowed to connect
                cGate *rightGateIn = curSat.getInputGate(Direction::ISL_RIGHT);
                cGate *leftGateIn = nextPlaneSat.getInputGate(Direction::ISL_LEFT);

                // generate or update ISL channel from right to left sat
                ChannelState state1 = updateOrCreateChannel(rightGateOut, leftGateIn, delay, islDatarate);
                ChannelState state2 = updateOrCreateChannel(leftGateOut, rightGateIn, delay, islDatarate);

                // if any channel was created, we have a topologyupdate
                if (state1 == ChannelState::CREATED || state2 == ChannelState::CREATED) {
                    curSat.rightSatellite = rightIndex;
                    nextPlaneSat.leftSatellite = index;
                    topologyChanged = true;
                }
            } else {  // they are not allowed to have an connection
                ChannelState state1 = deleteChannel(rightGateOut);
                ChannelState state2 = deleteChannel(leftGateOut);

                // if any channel was deleted, we have a topologyupdate
                if (state1 == ChannelState::DELETED || state2 == ChannelState::DELETED) {
                    curSat.rightSatellite = -1;
                    nextPlaneSat.leftSatellite = -1;
                    topologyChanged = true;
                }
            }
        } else {  // sat is moving down
            cGate *leftGateOut = curSat.getOutputGate(Direction::ISL_LEFT);
            cGate *rightGateOut = nextPlaneSat.getOutputGate(Direction::ISL_RIGHT);

            if (!nextPlaneSat.isAscending() && isIslEnabled(curSat) && isIslEnabled(nextPlaneSat)) {  // they are allowed to connect
                cGate *leftGateIn = curSat.getInputGate(Direction::ISL_LEFT);
                cGate *rightGateIn = nextPlaneSat.getInputGate(Direction::ISL_RIGHT);

                // generate or update ISL channel from right to left sat
                ChannelState state1 = updateOrCreateChannel(leftGateOut, rightGateIn, delay, islDatarate);
                ChannelState state2 = updateOrCreateChannel(rightGateOut, leftGateIn, delay, islDatarate);

                // if any channel was created, we have a topologyupdate
                if (state1 == ChannelState::CREATED || state2 == ChannelState::CREATED) {
                    curSat.leftSatellite = rightIndex;
                    nextPlaneSat.rightSatellite = index;
                    topologyChanged = true;
                }
            } else {  // they are not allowed to have an connection
                ChannelState state1 = deleteChannel(leftGateOut);
                ChannelState state2 = deleteChannel(rightGateOut);

                // if any channel was deleted, we have a topologyupdate
                if (state1 == ChannelState::DELETED || state2 == ChannelState::DELETED) {
                    curSat.leftSatellite = -1;
                    nextPlaneSat.rightSatellite = -1;
                    topologyChanged = true;
                }
            }
        }
    }
}

bool TopologyControl::isIslEnabled(PositionAwareBase &entity) const {
    double latitude = entity.getLatitude();
    return latitude <= upperLatitudeBound && latitude >= lowerLatitudeBound;
}

ChannelState TopologyControl::updateOrCreateChannel(cGate *outGate, cGate *inGate, double delay, double datarate) {
    if (outGate->isConnectedOutside()) {
        cDatarateChannel *channel = check_and_cast<cDatarateChannel *>(outGate->getChannel());
        channel->setDelay(delay);
        channel->setDatarate(datarate);
        return ChannelState::UPDATED;
    } else {
        cDatarateChannel *channel = cDatarateChannel::create(Constants::ISL_CHANNEL_NAME);
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
}

}  // namespace topologycontrol
}  // namespace flora
