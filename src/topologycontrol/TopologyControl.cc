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

TopologyControl::TopologyControl() : updateIntervalParameter(0),
                                     updateTimer(nullptr),
                                     numSatellites(0),
                                     numGroundStations(0),
                                     numGroundLinks(40),
                                     interPlaneIslDisabled(false),
                                     islDelay(0.0),
                                     islDatarate(0.0),
                                     groundlinkDelay(0.0),
                                     groundlinkDatarate(0.0),
                                     minimumElevation(10.0),
                                     walkerType(WalkerType::UNINITIALIZED),
                                     interPlaneSpacing(1),
                                     planeCount(0),
                                     satsPerPlane(0),
                                     topologyChanged(false) {
}

TopologyControl::~TopologyControl() {
    cancelAndDeleteClockEvent(updateTimer);
}

void TopologyControl::initialize(int stage) {
    if (stage == inet::INITSTAGE_LOCAL) {
        // loaded properties
        updateIntervalParameter = &par("updateInterval");
        updateTimer = new ClockEvent("UpdateTimer");
        walkerType = WalkerType::parseWalkerType(par("walkerType"));
        interPlaneIslDisabled = par("interPlaneIslDisabled");
        islDelay = par("islDelay");
        islDatarate = par("islDatarate");
        groundlinkDelay = par("groundlinkDelay");
        groundlinkDatarate = par("groundlinkDatarate");
        numGroundLinks = par("numGroundLinks");
        minimumElevation = par("minimumElevation");
        interPlaneSpacing = par("interPlaneSpacing");
        planeCount = par("planeCount");
        numSatellites = getSystemModule()->getSubmoduleVectorSize("loRaGW");
        numGroundStations = getSystemModule()->getSubmoduleVectorSize("groundStation");

        // calculated properties
        satsPerPlane = numSatellites / planeCount;

        // validate state
        VALIDATE(walkerType != WalkerType::UNINITIALIZED);
        VALIDATE(numGroundLinks > 0);
        VALIDATE(interPlaneSpacing <= planeCount - 1 && interPlaneSpacing >= 0);
        VALIDATE(numSatellites > 0);
        VALIDATE(planeCount > 0);
        VALIDATE(satsPerPlane > 0);
        VALIDATE(satsPerPlane * planeCount == numSatellites);
        VALIDATE(numGroundStations > 0);

    } else if (stage == inet::INITSTAGE_PHYSICAL_LAYER) {
        loadSatellites();
        loadGroundstations();

        if (satellites.size() == 0) {
            error("Error in TopologyControl::initialize(): No satellites found.");
            return;
        }

        updateTopology();

        if (!updateTimer->isScheduled()) {
            scheduleUpdate();
        }
    }
}

void TopologyControl::handleMessage(cMessage *msg) {
    if (msg == updateTimer) {
        updateTopology();
        scheduleUpdate();
    } else
        error("TopologyControl: Unknown message.");
}

void TopologyControl::scheduleUpdate() {
    scheduleClockEventAfter(updateIntervalParameter->doubleValue(), updateTimer);
}

void TopologyControl::updateTopology() {
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

GroundstationInfo const &TopologyControl::getGroundstationInfo(int gsId) const {
    ASSERT(gsId >= 0 && gsId < numGroundStations);
    return groundstationInfos.at(gsId);
}

SatelliteRoutingBase *const TopologyControl::getSatellite(int satId) const {
    ASSERT(satId >= 0 && satId < numSatellites);

    return satellites.at(satId);
}

std::unordered_map<int, SatelliteRoutingBase *> const &TopologyControl::getSatellites() const {
    return satellites;
}

GsSatConnection const &TopologyControl::getGroundstationSatConnection(int gsId, int satId) const {
    return gsSatConnections.at(std::pair<int, int>(gsId, satId));
}

void TopologyControl::loadGroundstations() {
    groundstationInfos.clear();
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
    satellites.clear();
    for (size_t i = 0; i < numSatellites; i++) {
        SatelliteRoutingBase *sat = check_and_cast<SatelliteRoutingBase *>(getParentModule()->getSubmodule("loRaGW", i));
        satellites.emplace(i, sat);
    }
}

void TopologyControl::updateIntraSatelliteLinks() {
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

void TopologyControl::updateGroundstationLinks() {
    // iterate over groundstations
    for (size_t gsId = 0; gsId < numGroundStations; gsId++) {
        GroundstationInfo &gsInfo = groundstationInfos.at(gsId);
        for (size_t satId = 0; satId < numSatellites; satId++) {
            SatelliteRoutingBase *sat = satellites.at(satId);

            bool isOldConnection = utils::set::contains<int>(gsInfo.satellites, satId);

            // if is not in range continue with next sat
            if (sat->getElevation(gsInfo) < minimumElevation) {
                // if they were previous connected, delete that connection
                if (isOldConnection) {
                    GsSatConnection &connection = gsSatConnections.at(std::pair<int, int>(gsId, satId));
                    cGate *uplink = gsInfo.getOutputGate(connection.gsGateIndex);
                    cGate *downlink = sat->getOutputGate(isldirection::Direction::ISL_DOWNLINK, connection.satGateIndex);
                    deleteChannel(uplink);
                    deleteChannel(downlink);
                    gsSatConnections.erase(std::pair<int, int>(gsId, satId));
                    gsInfo.satellites.erase(satId);
                    topologyChanged = true;
                }
                // in all cases continue with next sat
                continue;
            }

            double delay = sat->getDistance(gsInfo) * groundlinkDelay;

            // if they were previous connected, update channel with new delay
            if (isOldConnection) {
                GsSatConnection &connection = gsSatConnections.at(std::pair<int, int>(gsId, satId));
                cGate *uplinkO = gsInfo.getOutputGate(connection.gsGateIndex);
                cGate *uplinkI = gsInfo.getInputGate(connection.gsGateIndex);
                cGate *downlinkO = sat->getOutputGate(isldirection::Direction::ISL_DOWNLINK, connection.satGateIndex);
                cGate *downlinkI = sat->getInputGate(isldirection::Direction::ISL_DOWNLINK, connection.satGateIndex);
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
                    const cGate *gate = sat->getOutputGate(isldirection::Direction::ISL_DOWNLINK, i);
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
                cGate *downlinkO = sat->getOutputGate(isldirection::Direction::ISL_DOWNLINK, freeIndexSat);
                cGate *downlinkI = sat->getInputGate(isldirection::Direction::ISL_DOWNLINK, freeIndexSat);
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

SatelliteRoutingBase *TopologyControl::findSatByPlaneAndNumberInPlane(int plane, int numberInPlane) {
    int id = plane * satsPerPlane + numberInPlane;
    return satellites.at(id);
}

void TopologyControl::updateISLInWalkerDelta() {
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

void TopologyControl::updateISLInWalkerStar() {
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

void TopologyControl::connectSatellites(SatelliteRoutingBase *first, SatelliteRoutingBase *second, isldirection::Direction firstOutDir) {
    ASSERT(first != nullptr);
    ASSERT(second != nullptr);
    ASSERT(firstOutDir != isldirection::ISL_DOWNLINK);

    isldirection::Direction secondOutDir = isldirection::getCounterDirection(firstOutDir);

    cGate *firstOut = first->getOutputGate(firstOutDir);
    cGate *firstIn = firstOut->getOtherHalf();
    cGate *secondOut = second->getOutputGate(secondOutDir);
    cGate *secondIn = secondOut->getOtherHalf();

    ASSERT(firstOut != nullptr);
    ASSERT(firstIn != nullptr);
    ASSERT(secondOut != nullptr);
    ASSERT(secondIn != nullptr);

    // disconnect old connections if not the desired connections
    removeOldConnections(first, second, firstOutDir);

    double distance = first->getDistance(*second);
    double delay = islDelay * distance;

    ChannelState cs1 = updateOrCreateChannel(firstOut, secondIn, delay, islDatarate);
    ChannelState cs2 = updateOrCreateChannel(secondOut, firstIn, delay, islDatarate);

    switch (firstOutDir) {
        case isldirection::Direction::ISL_LEFT:
            first->setLeftSat(second);
            second->setRightSat(first);
            break;
        case isldirection::Direction::ISL_UP:
            first->setUpSat(second);
            second->setDownSat(first);
            break;
        case isldirection::Direction::ISL_RIGHT:
            first->setRightSat(second);
            second->setLeftSat(first);
            break;
        case isldirection::Direction::ISL_DOWN:
            first->setDownSat(second);
            second->setUpSat(first);
            break;
        default:
            error("Error in TopologyControl::connectSatellites: Should not reach default branch of switch.");
            break;
    }

    if (cs1 == ChannelState::CREATED || cs2 == ChannelState::CREATED) {
        topologyChanged = true;
    }
}

void TopologyControl::removeOldConnections(SatelliteRoutingBase *first, SatelliteRoutingBase *second, isldirection::Direction direction) {
    switch (direction) {
        case isldirection::Direction::ISL_LEFT:
            if (first->hasLeftSat() && first->getLeftSatId() != second->getId()) {
                disconnectSatellites(first, first->getLeftSat(), direction);
            }
            break;
        case isldirection::Direction::ISL_UP:
            if (first->hasUpSat() && first->getUpSatId() != second->getId()) {
                disconnectSatellites(first, first->getUpSat(), direction);
            }
            break;
        case isldirection::Direction::ISL_RIGHT:
            if (first->hasRightSat() && first->getRightSatId() != second->getId()) {
                disconnectSatellites(first, first->getRightSat(), direction);
            }
            break;
        case isldirection::Direction::ISL_DOWN:
            if (first->hasDownSat() && first->getDownSatId() != second->getId()) {
                disconnectSatellites(first, first->getDownSat(), direction);
            }
            break;
        default:
            break;
    }
}

void TopologyControl::disconnectSatellites(SatelliteRoutingBase *first, SatelliteRoutingBase *second, isldirection::Direction firstOutDir) {
    ASSERT(firstOutDir != isldirection::ISL_DOWNLINK);

    cGate *firstOut = first->getOutputGate(firstOutDir);
    cGate *secondOut = second->getOutputGate(isldirection::getCounterDirection(firstOutDir));

    ASSERT(firstOut != nullptr);
    ASSERT(secondOut != nullptr);

    ChannelState cs1 = deleteChannel(firstOut);
    ChannelState cs2 = deleteChannel(secondOut);

    if (cs1 == ChannelState::DELETED || cs2 == ChannelState::DELETED) {
        switch (firstOutDir) {
            case isldirection::Direction::ISL_LEFT:
                first->removeLeftSat();
                second->removeRightSat();
                break;
            case isldirection::Direction::ISL_UP:
                first->removeUpSat();
                second->removeDownSat();
                break;
            case isldirection::Direction::ISL_RIGHT:
                first->removeRightSat();
                second->removeLeftSat();
                break;
            case isldirection::Direction::ISL_DOWN:
                first->removeDownSat();
                second->removeUpSat();
                break;
            default:
                error("Error in TopologyControl::connectSatellites: Should not reach default branch of switch.");
                break;
        }
        topologyChanged = true;
    }
}

ChannelState TopologyControl::updateOrCreateChannel(cGate *outGate, cGate *inGate, double delay, double datarate) {
    ASSERT(outGate != nullptr);
    ASSERT(inGate != nullptr);
    ASSERT(delay > 0.0);
    ASSERT(datarate > 0.0);

    cGate *nextGate = outGate->getNextGate();
    if (nextGate != nullptr && nextGate->getId() == inGate->getId()) {
        cDatarateChannel *channel = check_and_cast<cDatarateChannel *>(outGate->getChannel());
        channel->setDelay(delay);
        channel->setDatarate(datarate);
        return ChannelState::UPDATED;
    } else {
        ASSERT(!outGate->isConnectedOutside());
        cDatarateChannel *channel = cDatarateChannel::create(Constants::ISL_CHANNEL_NAME);
        channel->setDelay(delay);
        channel->setDatarate(datarate);
        outGate->connectTo(inGate, channel);
        channel->callInitialize();
        return ChannelState::CREATED;
    }
}

void TopologyControl::disconnectGate(cGate *gate) {
    if (gate->getType() == cGate::Type::INPUT) {
        deleteChannel(gate->getPreviousGate());
        deleteChannel(gate->getOtherHalf());
    } else {
        cGate *nextGate = gate->getNextGate();
        if (nextGate != nullptr) {
            deleteChannel(nextGate->getOtherHalf());
            deleteChannel(gate);
        }
    }
}

ChannelState TopologyControl::deleteChannel(cGate *outGate) {
    if (outGate != nullptr && outGate->isConnectedOutside()) {
        outGate->disconnect();
        return ChannelState::DELETED;
    }
    return ChannelState::UNCHANGED;
}

void TopologyControl::trackTopologyChange() {
    EV << "Topology was changed at " << simTime() << endl;
}

}  // namespace topologycontrol
}  // namespace flora
