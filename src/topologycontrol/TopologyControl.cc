/*
 * TopologyControl.cc
 *
 * Created on: Dec 20, 2022
 *     Author: Robin Ohs
 */

#include "TopologyControl.h"

namespace flora
{
    namespace topologycontrol
    {
        Define_Module(TopologyControl);

        TopologyControl::TopologyControl() : updateTimer(nullptr),
                                             isClosedConstellation(false),
                                             updateInterval(0),
                                             islDatarate(0.0),
                                             islDelay(0.0),
                                             minimumElevation(10.0),
                                             walkerType(WalkerType::UNINITIALIZED)
        {
        }

        TopologyControl::~TopologyControl()
        {
            cancelAndDelete(updateTimer);
        }

        void TopologyControl::initialize(int stage)
        {
            if (stage == 0)
            {
                updateInterval = par("updateInterval");
                walkerType = WalkerType::parseWalkerType(par("walkerType"));
                isClosedConstellation = par("isClosedConstellation");
                lowerLatitudeBound = par("lowerLatitudeBound");
                upperLatitudeBound = par("upperLatitudeBound");
                islDelay = par("islDelay");
                islDatarate = par("islDatarate");
                groundlinkDelay = par("groundlinkDelay");
                groundlinkDatarate = par("groundlinkDatarate");
                minimumElevation = par("minimumElevation");
                EV << "Load parameters: "
                   << "updateInterval: " << updateInterval << "; "
                   << "isClosedConstellation: " << isClosedConstellation << "; "
                   << "lowerLatitudeBound: " << lowerLatitudeBound << "; "
                   << "upperLatitudeBound: " << upperLatitudeBound << "; "
                   << "islDelay: " << islDelay << "; "
                   << "islDatarate: " << islDatarate << "; "
                   << "minimumElevation: " << minimumElevation << endl;
                return;
            }
            EV << "initialize TopologyControl" << endl;

            loadSatellites();
            loadGroundstations();

            // take first satellite and read number of planes + satellitesPerPlane
            planeCount = satelliteInfos.at(0).noradModule->getNumberOfPlanes();
            satsPerPlane = satelliteInfos.at(0).noradModule->getSatellitesPerPlane();

            UpdateTopology();
            updateTimer = new cMessage("update");
            scheduleUpdate();
        }

        void TopologyControl::handleMessage(cMessage *msg)
        {
            if (msg->isSelfMessage())
                handleSelfMessage(msg);
            else
                throw cRuntimeError("TopologyControl module can only receive self messages");
        }

        void TopologyControl::handleSelfMessage(cMessage *msg)
        {
            EV << "Update topology" << endl;
            UpdateTopology();
            scheduleUpdate();
        }

        void TopologyControl::scheduleUpdate()
        {
            cancelEvent(updateTimer);
            if (updateInterval != 0)
            {
                simtime_t nextUpdate = simTime() + updateInterval;
                scheduleAt(nextUpdate, updateTimer);
            }
        }

        void TopologyControl::UpdateTopology()
        {
            if (satelliteInfos.size() == 0)
            {
                error("Error in TopologyControl::UpdateTopology(): No satellites found.");
                return;
            }
            // update ISL links and groundlinks
            topologyChanged = false;
            updateIntraSatelliteLinks();
            updateInterSatelliteLinks();
            updateGroundstationLinks();

            // if there was any change to the topology, track current contacts
            if (topologyChanged)
                trackTopologyChange();
        }

        GroundstationInfo *TopologyControl::getGroundstationInfo(int gsId)
        {
            if (gsId < 0 || gsId >= satelliteCount)
            {
                error("Error in TopologyControl::getGroundStationInfo: '%d' must be in range [0, %d)", gsId, satelliteCount);
            }
            return &groundstationInfos.at(gsId);
        }

        void TopologyControl::loadGroundstations()
        {
            groundstationInfos.clear();
            groundstationCount = getSystemModule()->getSubmoduleVectorSize("groundStation");
            for (size_t i = 0; i < groundstationCount; i++)
            {
                cModule *groundstation = getSystemModule()->getSubmodule("groundStation", i);
                if (groundstation == nullptr)
                {
                    error("Error in TopologyControl::getGroundstations(): groundStation with index %zu is nullptr. Make sure the module exists.", i);
                }
                GroundStationMobility *mobility = check_and_cast<GroundStationMobility *>(groundstation->getSubmodule("mobility"));
                if (mobility == nullptr)
                {
                    error("Error in TopologyControl::getGroundstations(): mobility module of Groundstation is nullptr. Make sure a module with name `mobility` exists.");
                }
                GroundstationInfo created = GroundstationInfo(groundstation->par("groundStationId"), groundstation, mobility);
                groundstationInfos.emplace(i, created);
                EV << "Created GroundstationInfo" << created.to_string() << endl;
            }
        }

        void TopologyControl::loadSatellites()
        {
            satelliteInfos.clear();
            satelliteCount = getParentModule()->getSubmoduleVectorSize("loRaGW");
            for (size_t i = 0; i < satelliteCount; i++)
            {
                cModule *sat = getParentModule()->getSubmodule("loRaGW", i);
                if (sat == nullptr)
                {
                    error("Error in TopologyControl::getSatellites(): loRaGW with index %zu is nullptr. Make sure the module exists.", i);
                }
                NoradA *noradA = check_and_cast<NoradA *>(sat->getSubmodule("NoradModule"));
                if (noradA == nullptr)
                {
                    error("Error in TopologyControl::getSatellites(): noradA module of loRaGW with index %zu is nullptr. Make sure a module with name `NoradModule` exists.", i);
                }
                SatelliteInfo created = SatelliteInfo(i, sat, noradA);
                satelliteInfos.emplace(i, created);
            }
        }

        void TopologyControl::updateIntraSatelliteLinks()
        {
            // iterate over planes
            for (size_t plane = 0; plane < planeCount; plane++)
            {
                // iterate over sats in plane
                for (size_t planeSat = 0; planeSat < satsPerPlane; planeSat++)
                {
                    int index = planeSat + plane * satsPerPlane;
                    int isLastSatInPlane = index % satsPerPlane == satsPerPlane - 1;

                    // if its only a "slice" of a constellation, no connection between the first and the last sat
                    if (isLastSatInPlane && !isClosedConstellation)
                    {
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
                    if (state1 == ChannelState::CREATED || state2 == ChannelState::CREATED)
                    {
                        topologyChanged = true;
                        // Assign satellites in SatelliteInfo.
                        // Important: Assumption that intra satellite links are not gonna change after initial connect.
                        curSat->upSatellite = nextId;
                        otherSat->downSatellite = index;
                    }
                }
            }
        }

        void TopologyControl::updateGroundstationLinks()
        {
            // Connect nearest satellite
            for (size_t i = 0; i < groundstationCount; i++)
            {
                GroundstationInfo *gsInfo = &groundstationInfos.at(i);
                EV << gsInfo->to_string() << endl;

                // find nearest satellite with elevation >= minElevation
                SatelliteInfo *nearestSatellite = nullptr;
                double currentElevation = -99999;
                for (size_t i = 0; i < satelliteCount; i++)
                {
                    SatelliteInfo *satInfo = &satelliteInfos.at(i);
                    double elevation = ((INorad *)satInfo->noradModule)->getElevation(gsInfo->mobility->getLUTPositionY(), gsInfo->mobility->getLUTPositionX(), 0);
                    if ((nearestSatellite == nullptr || elevation > currentElevation) && elevation >= minimumElevation)
                    {
                        nearestSatellite = satInfo;
                        currentElevation = elevation;
                    }
                }

                // no nearest satellite and has old connection
                if (nearestSatellite == nullptr)
                {
                    if (gsInfo->satelliteId != -1)
                    {
                        SatelliteInfo *oldSatellite = &satelliteInfos.at(gsInfo->satelliteId);
                        cGate *uplink = gsInfo->groundStation->gateHalf(GS_SATLINK_NAME.c_str(), cGate::Type::OUTPUT);
                        cGate *downlink = oldSatellite->satelliteModule->gateHalf(SAT_GROUNDLINK_NAME.c_str(), cGate::Type::OUTPUT);
                        deleteChannel(uplink);
                        deleteChannel(downlink);
                        gsInfo->satelliteId = -1;
                        topologyChanged = true;
                    }
                    else
                    {
                        error("Found no satellite for groundstation %d.", gsInfo->groundStationId);
                    }
                }
                else
                {
                    EV << "Attempt to connect " << gsInfo->groundStationId << " and " << nearestSatellite->satelliteId << "." << endl;
                    // get the gates
                    cGate *uplinkO = gsInfo->groundStation->gateHalf(GS_SATLINK_NAME.c_str(), cGate::Type::OUTPUT);
                    cGate *uplinkI = gsInfo->groundStation->gateHalf(GS_SATLINK_NAME.c_str(), cGate::Type::INPUT);
                    cGate *downlinkO = nearestSatellite->satelliteModule->gateHalf(SAT_GROUNDLINK_NAME.c_str(), cGate::Type::OUTPUT);
                    cGate *downlinkI = nearestSatellite->satelliteModule->gateHalf(SAT_GROUNDLINK_NAME.c_str(), cGate::Type::INPUT);

                    // distance between nearest satellite and groundstation (km)
                    double distance = nearestSatellite->noradModule->getDistance(gsInfo->mobility->getLUTPositionY(), gsInfo->mobility->getLUTPositionX(), 0);
                    // delay of the channel between nearest satellite and groundstation (ms)
                    double delay = distance * groundlinkDelay;

                    // has nearest satellite and is the old one
                    if (gsInfo->satelliteId == nearestSatellite->satelliteId)
                    {
                        // update the channel
                        updateOrCreateChannel(uplinkO, downlinkI, delay, groundlinkDatarate);
                        updateOrCreateChannel(downlinkO, uplinkI, delay, groundlinkDatarate);
                    }
                    // has nearest satellite and is not the old one
                    else
                    {
                        // remove connection between previous nearest satellite and groundstation
                        if (gsInfo->satelliteId != -1)
                        {
                            SatelliteInfo *prevSat = &satelliteInfos.at(gsInfo->satelliteId);
                            cGate *prevGroundlinkO = prevSat->satelliteModule->gateHalf(SAT_GROUNDLINK_NAME.c_str(), cGate::Type::OUTPUT);
                            deleteChannel(prevGroundlinkO);
                            deleteChannel(uplinkO);
                        }

                        // disconnect new nearest satellite with its previous groundstation if there was a connection
                        if (nearestSatellite->groundStationId != -1)
                        {
                            GroundstationInfo *oldGs = &groundstationInfos.at(nearestSatellite->groundStationId);
                            cGate *oldGsUplinkO = gsInfo->groundStation->gateHalf(GS_SATLINK_NAME.c_str(), cGate::Type::OUTPUT);
                            deleteChannel(oldGsUplinkO);
                            deleteChannel(downlinkO);
                        }

                        // connect groundstation and nearest satellite
                        updateOrCreateChannel(uplinkO, downlinkI, delay, groundlinkDatarate);
                        updateOrCreateChannel(downlinkO, uplinkI, delay, groundlinkDatarate);
                        gsInfo->satelliteId = nearestSatellite->satelliteId;
                        topologyChanged = true;
                    }
                }

                EV << gsInfo->to_string() << endl;
            }

            // TODO: Support N satellites to M groundstations connections
        }

        void TopologyControl::updateInterSatelliteLinks()
        {
            switch (walkerType)
            {
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

        void TopologyControl::updateISLInWalkerDelta()
        {
            for (size_t index = 0; index < satelliteCount; index++)
            {
                SatelliteInfo *curSat = &satelliteInfos.at(index);
                int satPlane = calculateSatellitePlane(index);
                int nextPlane = (satPlane + 1) % planeCount;

                int nextPlaneSatIndex = (index + satsPerPlane) % satelliteCount;
                SatelliteInfo *expectedRightSat = &satelliteInfos.at(nextPlaneSatIndex);

                if (curSat->noradModule->isAscending())
                {
                    cGate *rightGateOut = curSat->satelliteModule->gateHalf(ISL_RIGHT_NAME.c_str(), cGate::Type::OUTPUT);
                    cGate *leftGateOut = expectedRightSat->satelliteModule->gateHalf(ISL_LEFT_NAME.c_str(), cGate::Type::OUTPUT);

                    // if next plane partner is not ascending, connection is not possible
                    if (!expectedRightSat->noradModule->isAscending())
                    {
                        // if we were connected to that satellite on right
                        if (curSat->rightSatellite == nextPlaneSatIndex)
                        {
                            deleteChannel(rightGateOut);
                            deleteChannel(leftGateOut);
                            curSat->rightSatellite = -1;
                            expectedRightSat->leftSatellite = -1;
                            topologyChanged = true;
                        }
                        // if we were connected to that satellite on left
                        else if (curSat->leftSatellite == nextPlaneSatIndex)
                        {
                            cGate *leftGateOutOther = curSat->satelliteModule->gateHalf(ISL_LEFT_NAME.c_str(), cGate::Type::OUTPUT);
                            cGate *rightGateOutOther = expectedRightSat->satelliteModule->gateHalf(ISL_RIGHT_NAME.c_str(), cGate::Type::OUTPUT);
                            deleteChannel(leftGateOutOther);
                            deleteChannel(rightGateOutOther);
                            curSat->leftSatellite = -1;
                            expectedRightSat->rightSatellite = -1;
                            topologyChanged = true;
                        }
                    }
                    else
                    {
                        cGate *rightGateIn = curSat->satelliteModule->gateHalf(ISL_RIGHT_NAME.c_str(), cGate::Type::INPUT);
                        cGate *leftGateIn = expectedRightSat->satelliteModule->gateHalf(ISL_LEFT_NAME.c_str(), cGate::Type::INPUT);
                        // disconnect the other satellite if has left satellite and it is not ours
                        if (expectedRightSat->leftSatellite != index && expectedRightSat->leftSatellite != -1)
                        {
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
                        if (state1 == ChannelState::CREATED || state2 == ChannelState::CREATED)
                        {
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
                    if (expectedRightSat->noradModule->isAscending())
                    {
                        // if we were connected to that satellite on right
                        if (curSat->leftSatellite == nextPlaneSatIndex)
                        {
                            deleteChannel(leftGateOut);
                            deleteChannel(rightGateOut);
                            curSat->leftSatellite = -1;
                            expectedRightSat->rightSatellite = -1;
                            topologyChanged = true;
                        }
                        // if we were connected to that satellite on right
                        else if (curSat->rightSatellite == nextPlaneSatIndex)
                        {
                            cGate *rightGateOutOther = curSat->satelliteModule->gateHalf(ISL_RIGHT_NAME.c_str(), cGate::Type::OUTPUT);
                            cGate *leftGateOutOther = expectedRightSat->satelliteModule->gateHalf(ISL_LEFT_NAME.c_str(), cGate::Type::OUTPUT);
                            deleteChannel(rightGateOutOther);
                            deleteChannel(leftGateOutOther);
                            curSat->rightSatellite = -1;
                            expectedRightSat->leftSatellite = -1;
                            topologyChanged = true;
                        }
                    }
                    else
                    {
                        cGate *leftGateIn = curSat->satelliteModule->gateHalf(ISL_LEFT_NAME.c_str(), cGate::Type::INPUT);
                        cGate *rightGateIn = expectedRightSat->satelliteModule->gateHalf(ISL_RIGHT_NAME.c_str(), cGate::Type::INPUT);
                        
                        // disconnect the other satellite if has right satellite and it is not ours
                        if (expectedRightSat->rightSatellite != index && expectedRightSat->rightSatellite != -1)
                        {
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
                        if (state1 == ChannelState::CREATED || state2 == ChannelState::CREATED)
                        {
                            curSat->leftSatellite = nextPlaneSatIndex;
                            expectedRightSat->rightSatellite = index;
                            topologyChanged = true;
                        }
                    }
                }
            }
        }

        void TopologyControl::updateISLInWalkerStar()
        {
            for (size_t index = 0; index < satelliteCount; index++)
            {
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

                if (curSat->noradModule->isAscending())
                { // sat is moving up
                    cGate *rightGateOut = curSat->satelliteModule->gateHalf(ISL_RIGHT_NAME.c_str(), cGate::Type::OUTPUT);
                    cGate *leftGateOut = nextPlaneSat->satelliteModule->gateHalf(ISL_LEFT_NAME.c_str(), cGate::Type::OUTPUT);

                    if (nextPlaneSat->noradModule->isAscending() && isIslEnabled(curSat->noradModule->getLatitude()) && isIslEnabled(nextPlaneSat->noradModule->getLatitude()))
                    { // they are allowed to connect
                        cGate *rightGateIn = curSat->satelliteModule->gateHalf(ISL_RIGHT_NAME.c_str(), cGate::Type::INPUT);
                        cGate *leftGateIn = nextPlaneSat->satelliteModule->gateHalf(ISL_LEFT_NAME.c_str(), cGate::Type::INPUT);

                        // generate or update ISL channel from right to left sat
                        ChannelState state1 = updateOrCreateChannel(rightGateOut, leftGateIn, delay, islDatarate);
                        ChannelState state2 = updateOrCreateChannel(leftGateOut, rightGateIn, delay, islDatarate);

                        // if any channel was created, we have a topologyupdate
                        if (state1 == ChannelState::CREATED || state2 == ChannelState::CREATED)
                        {
                            curSat->rightSatellite = rightIndex;
                            nextPlaneSat->leftSatellite = index;
                            topologyChanged = true;
                        }
                    }
                    else
                    { // they are not allowed to have an connection
                        ChannelState state1 = deleteChannel(rightGateOut);
                        ChannelState state2 = deleteChannel(leftGateOut);

                        // if any channel was deleted, we have a topologyupdate
                        if (state1 == ChannelState::DELETED || state2 == ChannelState::DELETED)
                        {
                            curSat->rightSatellite = -1;
                            nextPlaneSat->leftSatellite = -1;
                            topologyChanged = true;
                        }
                    }
                }
                else
                { // sat is moving down
                    cGate *leftGateOut = curSat->satelliteModule->gateHalf(ISL_LEFT_NAME.c_str(), cGate::Type::OUTPUT);
                    cGate *rightGateOut = nextPlaneSat->satelliteModule->gateHalf(ISL_RIGHT_NAME.c_str(), cGate::Type::OUTPUT);

                    if (!nextPlaneSat->noradModule->isAscending() && isIslEnabled(curSat->noradModule->getLatitude()) && isIslEnabled(nextPlaneSat->noradModule->getLatitude()))
                    { // they are allowed to connect
                        cGate *leftGateIn = curSat->satelliteModule->gateHalf(ISL_LEFT_NAME.c_str(), cGate::Type::INPUT);
                        cGate *rightGateIn = nextPlaneSat->satelliteModule->gateHalf(ISL_RIGHT_NAME.c_str(), cGate::Type::INPUT);

                        // generate or update ISL channel from right to left sat
                        ChannelState state1 = updateOrCreateChannel(leftGateOut, rightGateIn, delay, islDatarate);
                        ChannelState state2 = updateOrCreateChannel(rightGateOut, leftGateIn, delay, islDatarate);

                        // if any channel was created, we have a topologyupdate
                        if (state1 == ChannelState::CREATED || state2 == ChannelState::CREATED)
                        {
                            curSat->leftSatellite = rightIndex;
                            nextPlaneSat->rightSatellite = index;
                            topologyChanged = true;
                        }
                    }
                    else
                    { // they are not allowed to have an connection
                        ChannelState state1 = deleteChannel(leftGateOut);
                        ChannelState state2 = deleteChannel(rightGateOut);

                        // if any channel was deleted, we have a topologyupdate
                        if (state1 == ChannelState::DELETED || state2 == ChannelState::DELETED)
                        {
                            curSat->leftSatellite = -1;
                            nextPlaneSat->rightSatellite = -1;
                            topologyChanged = true;
                        }
                    }
                }
            }
        }

        bool TopologyControl::isIslEnabled(double latitude)
        {
            return latitude <= upperLatitudeBound && latitude >= lowerLatitudeBound;
        }

        ChannelState TopologyControl::updateOrCreateChannel(cGate *outGate, cGate *inGate, double delay, double datarate)
        {
            if (outGate->isConnectedOutside())
            {
                cDatarateChannel *channel = check_and_cast<cDatarateChannel *>(outGate->getChannel());
                channel->setDelay(delay);
                channel->setDatarate(datarate);
                return ChannelState::UPDATED;
            }
            else
            {
                cDatarateChannel *channel = cDatarateChannel::create(ISL_CHANNEL_NAME.c_str());
                channel->setDelay(delay);
                channel->setDatarate(datarate);
                outGate->connectTo(inGate, channel);
                channel->callInitialize();
                return ChannelState::CREATED;
            }
        }

        ChannelState TopologyControl::deleteChannel(cGate *outGate)
        {
            if (outGate->isConnectedOutside())
            {
                outGate->disconnect();
                return ChannelState::DELETED;
            }
            return ChannelState::UNCHANGED;
        }

        int TopologyControl::calculateSatellitePlane(int id)
        {
            int satPlane = (id - (id % satsPerPlane)) / satsPerPlane;
            if (satPlane > planeCount - 1)
            {
                error("Error in TopologyControl::calculateSatellitePlane(): Calculated plane (%d) is bigger than number of planes - 1 (%d).", satPlane, planeCount - 1);
            }
            return satPlane;
        }

        void TopologyControl::trackTopologyChange()
        {
            EV << "Topology was changed at " << simTime() << endl;
            for (size_t i = 0; i < satelliteCount; i++)
            {
                SatelliteInfo *satInfo = &satelliteInfos.at(i);
                EV << satInfo->to_string() << endl;
            }
        }
    } // topologycontrol
} // flora
