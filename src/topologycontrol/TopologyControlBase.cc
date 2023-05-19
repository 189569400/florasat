/*
 * TopologyControlBase.h
 *
 * Created on: May 05, 2023
 *     Author: Robin Ohs
 */

#include "TopologyControlBase.h"

namespace flora {
namespace topologycontrol {

TopologyControlBase::TopologyControlBase() : updateIntervalParameter(0),
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

TopologyControlBase::~TopologyControlBase() {
    cancelAndDeleteClockEvent(updateTimer);
}

void TopologyControlBase::initialize(int stage) {
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
        EV << "TC: Loaded parameters: "
           << "updateInterval: " << updateIntervalParameter << "; "
           << "islDelay: " << islDelay << "; "
           << "islDatarate: " << islDatarate << "; "
           << "minimumElevation: " << minimumElevation << endl;
    } else if (stage == inet::INITSTAGE_PHYSICAL_LAYER) {
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

void TopologyControlBase::handleMessage(cMessage *msg) {
    if (msg == updateTimer) {
        updateTopology();
        scheduleUpdate();
    } else
        error("TopologyControlBase: Unknown message.");
}

void TopologyControlBase::scheduleUpdate() {
    scheduleClockEventAfter(updateIntervalParameter->doubleValue(), updateTimer);
}

GroundstationInfo const &TopologyControlBase::getGroundstationInfo(int gsId) const {
    ASSERT(gsId >= 0 && gsId < numGroundStations);
    return groundstationInfos.at(gsId);
}

SatelliteRoutingBase *const TopologyControlBase::getSatellite(int satId) const {
    ASSERT(satId >= 0 && satId < numSatellites);
    return satellites.at(satId);
}

std::unordered_map<int, SatelliteRoutingBase *> const &TopologyControlBase::getSatellites() const {
    return satellites;
}

GsSatConnection const &TopologyControlBase::getGroundstationSatConnection(int gsId, int satId) const {
    return gsSatConnections.at(std::pair<int, int>(gsId, satId));
}

SatelliteRoutingBase *TopologyControlBase::findSatByPlaneAndNumberInPlane(int plane, int numberInPlane) {
    int id = plane * satsPerPlane + numberInPlane;
    return satellites.at(id);
}

void TopologyControlBase::loadGroundstations() {
    groundstationInfos.clear();
    for (size_t i = 0; i < numGroundStations; i++) {
        cModule *groundstation = getSystemModule()->getSubmodule("groundStation", i);
        if (groundstation == nullptr) {
            error("Error in TopologyControlBase::getGroundstations(): groundStation with index %zu is nullptr. Make sure the module exists.", i);
        }
        GroundStationMobility *mobility = check_and_cast<GroundStationMobility *>(groundstation->getSubmodule("gsMobility"));
        if (mobility == nullptr) {
            error("Error in TopologyControlBase::getGroundstations(): mobility module of Groundstation is nullptr. Make sure a module with name `gsMobility` exists.");
        }
        GroundstationInfo created = GroundstationInfo(groundstation->par("groundStationId"), groundstation, mobility);
        groundstationInfos.emplace(i, created);
    }
}

void TopologyControlBase::loadSatellites() {
    satellites.clear();
    for (size_t i = 0; i < numSatellites; i++) {
        SatelliteRoutingBase *sat = check_and_cast<SatelliteRoutingBase *>(getParentModule()->getSubmodule("loRaGW", i));
        satellites.emplace(i, sat);
    }
}

void TopologyControlBase::connectSatellites(SatelliteRoutingBase *first, SatelliteRoutingBase *second, isldirection::Direction direction) {
    ASSERT(first != nullptr);
    ASSERT(second != nullptr);
    ASSERT(direction != isldirection::ISL_DOWNLINK);

    isldirection::Direction counterDirection = isldirection::getCounterDirection(direction);

    cGate *firstOut = first->getOutputGate(direction).first;
    cGate *firstIn = first->getInputGate(direction).first;
    cGate *secondOut = second->getOutputGate(counterDirection).first;
    cGate *secondIn = second->getInputGate(counterDirection).first;

    ASSERT(firstOut != nullptr);
    ASSERT(firstIn != nullptr);
    ASSERT(secondOut != nullptr);
    ASSERT(secondIn != nullptr);

#ifndef NDEBUG
    EV << "<><><><><><><><><><><><>" << endl;
    EV << "Connect " << first->getId() << " and " << second->getId() << " on " << to_string(direction) << endl;
#endif

    // disconnect old connections if not the desired connections
    removeOldConnections(first, second, direction);

    double distance = first->getDistance(*second);
    double delay = islDelay * distance;

    ChannelState cs1 = updateOrCreateChannel(firstOut, secondIn, delay, islDatarate);
    ChannelState cs2 = updateOrCreateChannel(secondOut, firstIn, delay, islDatarate);

    ASSERT(cs1 == cs2);

    if (cs1 == ChannelState::CREATED && cs2 == ChannelState::CREATED) {
        switch (direction) {
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
                error("Error in TopologyControlBase::connectSatellites: Should not reach default branch of switch.");
                break;
        }
        topologyChanged = true;
    }

#ifndef NDEBUG
    EV << "<X><X><X><X><X><X>" << endl;
#endif
}

void TopologyControlBase::createConnection(SatelliteRoutingBase *from, SatelliteRoutingBase *to, isldirection::Direction direction) {
    ASSERT(from != nullptr);
    ASSERT(to != nullptr);
    ASSERT(direction != isldirection::Direction::ISL_DOWNLINK);
}

void TopologyControlBase::removeOldConnections(SatelliteRoutingBase *first, SatelliteRoutingBase *second, isldirection::Direction direction) {
    switch (direction) {
        case isldirection::Direction::ISL_LEFT:
            if (first->hasLeftSat() && first->getLeftSatId() != second->getId()) {
                disconnectSatellites(first, first->getLeftSat(), direction);
                // remove potential connection of new partner
                if (second->hasRightSat()) {
                    disconnectSatellites(second, second->getRightSat(), isldirection::Direction::ISL_RIGHT);
                }
            }
            break;
        case isldirection::Direction::ISL_UP:
            if (first->hasUpSat() && first->getUpSatId() != second->getId()) {
                disconnectSatellites(first, first->getUpSat(), direction);
                // remove potential connection of new partner
                if (second->hasDownSat()) {
                    disconnectSatellites(second, second->getDownSat(), isldirection::Direction::ISL_DOWN);
                }
            }
            break;
        case isldirection::Direction::ISL_RIGHT:
            if (first->hasRightSat() && first->getRightSatId() != second->getId()) {
                disconnectSatellites(first, first->getRightSat(), direction);
                // remove potential connection of new partner
                if (second->hasLeftSat()) {
                    disconnectSatellites(second, second->getLeftSat(), isldirection::Direction::ISL_LEFT);
                }
            }
            break;
        case isldirection::Direction::ISL_DOWN:
            if (first->hasDownSat() && first->getDownSatId() != second->getId()) {
                disconnectSatellites(first, first->getDownSat(), direction);
                // remove potential connection of new partner
                if (second->hasUpSat()) {
                    disconnectSatellites(second, second->getUpSat(), isldirection::Direction::ISL_UP);
                }
            }
            break;
        default:
            break;
    }
}

void TopologyControlBase::disconnectSatellites(SatelliteRoutingBase *first, SatelliteRoutingBase *second, isldirection::Direction firstOutDir) {
    ASSERT(firstOutDir != isldirection::ISL_DOWNLINK);

#ifndef NDEBUG
    EV << "Disconnect " << first->getId() << " and " << second->getId() << " on " << to_string(firstOutDir) << endl;
#endif

    cGate *firstOut = first->getOutputGate(firstOutDir).first;
    cGate *secondOut = second->getOutputGate(isldirection::getCounterDirection(firstOutDir)).first;

    ASSERT(firstOut != nullptr);
    ASSERT(secondOut != nullptr);

    ChannelState cs1 = deleteChannel(firstOut);
    ChannelState cs2 = deleteChannel(secondOut);

    if (cs1 == ChannelState::DELETED) {
        switch (firstOutDir) {
            case isldirection::Direction::ISL_LEFT:
                first->removeLeftSat();
                break;
            case isldirection::Direction::ISL_UP:
                first->removeUpSat();
                break;
            case isldirection::Direction::ISL_RIGHT:
                first->removeRightSat();
                break;
            case isldirection::Direction::ISL_DOWN:
                first->removeDownSat();
                break;
            default:
                error("Error in TopologyControlBase::disconnectSatellites: Should not reach default branch of switch.");
                break;
        }
        topologyChanged = true;
    }

    if (cs2 == ChannelState::DELETED) {
        switch (firstOutDir) {
            case isldirection::Direction::ISL_LEFT:
                second->removeRightSat();
                break;
            case isldirection::Direction::ISL_UP:
                second->removeDownSat();
                break;
            case isldirection::Direction::ISL_RIGHT:
                second->removeLeftSat();
                break;
            case isldirection::Direction::ISL_DOWN:
                second->removeUpSat();
                break;
            default:
                error("Error in TopologyControlBase::disconnectSatellites: Should not reach default branch of switch.");
                break;
        }
        topologyChanged = true;
    }
}

ChannelState TopologyControlBase::updateOrCreateChannel(cGate *outGate, cGate *inGate, double delay, double datarate) {
    ASSERT(outGate != nullptr);
    ASSERT(inGate != nullptr);
    ASSERT(delay > 0.0);
    ASSERT(datarate > 0.0);

    cGate *nextGate = outGate->getNextGate();
    if (nextGate != nullptr && nextGate->getId() == inGate->getId()) {
        ASSERT(outGate->isConnectedOutside());
#ifndef NDEBUG
        EV << "Update channel: " << ((SatelliteRoutingBase *)outGate->getOwnerModule())->getId() << "->" << ((SatelliteRoutingBase *)inGate->getOwnerModule())->getId() << endl;
#endif
        cDatarateChannel *channel = check_and_cast<cDatarateChannel *>(outGate->getChannel());
        channel->setDelay(delay);
        channel->setDatarate(datarate);
        return ChannelState::UPDATED;
    } else {
        ASSERT(!outGate->isConnectedOutside());
#ifndef NDEBUG
        EV << "Create channel: " << ((SatelliteRoutingBase *)outGate->getOwnerModule())->getId() << "->" << ((SatelliteRoutingBase *)inGate->getOwnerModule())->getId() << endl;
#endif
        cDatarateChannel *channel = cDatarateChannel::create(Constants::ISL_CHANNEL_NAME);
        channel->setDelay(delay);
        channel->setDatarate(datarate);
        outGate->connectTo(inGate, channel);
        channel->callInitialize();
        return ChannelState::CREATED;
    }
}

ChannelState TopologyControlBase::deleteChannel(cGate *outGate) {
    if (outGate != nullptr && outGate->isConnectedOutside()) {
        outGate->disconnect();
        return ChannelState::DELETED;
    }
    return ChannelState::UNCHANGED;
}

}  // namespace topologycontrol
}  // namespace flora