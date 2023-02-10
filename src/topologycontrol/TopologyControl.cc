/*
 * TopologyControl.cc
 *
 * Created on: Dec 20, 2022
 *     Author: Robin Ohs
 */

#include "TopologyControl.h"

namespace flora
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
        satellites = getSatellites();
        // take first satellite and read number of planes + satellitesPerPlane
        planeCount = satellites.at(0).second->getNumberOfPlanes();
        satsPerPlane = satellites.at(0).second->getSatellitesPerPlane();

        groundstationInfos = getGroundstations();

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
        if (satellites.size() == 0)
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

    GroundstationInfo *TopologyControl::loadGroundstationInfo(int gsId)
    {
        for (GroundstationInfo &gsInfo : groundstationInfos)
        {
            if (gsInfo.groundStationId == gsId)
            {
                return &gsInfo;
            }
        }
        return nullptr;
    }

    std::vector<GroundstationInfo> TopologyControl::getGroundstations()
    {
        std::vector<GroundstationInfo> loadedGroundstations;
        int gsCount = getSystemModule()->getSubmoduleVectorSize("groundStation");
        for (size_t i = 0; i < gsCount; i++)
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
            loadedGroundstations.push_back(created);
            EV << "Created GroundstationInfo" << created.to_string() << endl;
        }

        return loadedGroundstations;
    }

    std::map<int, std::pair<cModule *, NoradA *>> TopologyControl::getSatellites()
    {
        std::map<int, std::pair<cModule *, NoradA *>> loadedSatellites;
        int satCount = getParentModule()->getSubmoduleVectorSize("loRaGW");
        for (size_t i = 0; i < satCount; i++)
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
            loadedSatellites.emplace(i, std::make_pair(sat, noradA));
        }
        return loadedSatellites;
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
                std::pair<cModule *, NoradA *> curSat = satellites.at(index);
                std::pair<cModule *, NoradA *> otherSat = isLastSatInPlane ? satellites.at(plane * satsPerPlane) : satellites.at(index + 1);

                // get gates
                cGate *fromGateIn = curSat.first->gateHalf("up", cGate::Type::INPUT);
                cGate *fromGateOut = curSat.first->gateHalf("up", cGate::Type::OUTPUT);
                cGate *toGateIn = otherSat.first->gateHalf("down", cGate::Type::INPUT);
                cGate *toGateOut = otherSat.first->gateHalf("down", cGate::Type::OUTPUT);

                // calculate ISL channel params
                double distance = curSat.second->getDistance(otherSat.second->getLatitude(), otherSat.second->getLongitude(), otherSat.second->getAltitude());
                double delay = islDelay * distance;

                // generate or update ISL channel from lower to upper sat
                updateOrCreateChannel(fromGateOut, toGateIn, delay, islDatarate);
                updateOrCreateChannel(toGateOut, fromGateIn, delay, islDatarate);
            }
        }
    }

    void TopologyControl::updateGroundstationLinks()
    {
        // Connect nearest satellite
        for (GroundstationInfo &gsInfo : groundstationInfos)
        {
            // find nearest satellite with elevation >= minElevation
            cModule *nearestSatellite = nullptr;
            INorad *nearestSatelliteNorad = nullptr;
            double currentElevation = -99999;
            for (size_t i = 0; i < satellites.size(); i++)
            {
                double elevation = ((INorad *)satellites.at(i).second)->getElevation(gsInfo.mobility->getLUTPositionY(), gsInfo.mobility->getLUTPositionX(), 0);
                if ((nearestSatellite == nullptr || elevation > currentElevation) && elevation >= minimumElevation)
                {
                    nearestSatellite = satellites.at(i).first;
                    nearestSatelliteNorad = satellites.at(i).second;
                    currentElevation = elevation;
                }
            }

            // no nearest satellite and has old connection
            if (nearestSatellite == nullptr && gsInfo.satellite != nullptr)
            {
                cGate *uplink = gsInfo.groundStation->gateHalf("satelliteLink", cGate::Type::OUTPUT);
                cGate *downlink = gsInfo.satellite->gateHalf("groundLink", cGate::Type::OUTPUT);
                deleteChannel(uplink);
                deleteChannel(downlink);
                gsInfo.satellite = nullptr;
                EV << "Groundstation disconnect: " << gsInfo.to_string() << endl;
                topologyChanged = true;
                continue;
            }

            // get the gates
            cGate *uplinkO = gsInfo.groundStation->gateHalf("satelliteLink", cGate::Type::OUTPUT);
            cGate *uplinkI = gsInfo.groundStation->gateHalf("satelliteLink", cGate::Type::INPUT);
            cGate *downlinkO = nearestSatellite->gateHalf("groundLink", cGate::Type::OUTPUT);
            cGate *downlinkI = nearestSatellite->gateHalf("groundLink", cGate::Type::INPUT);

            // distance between nearest satellite and groundstation (km)
            double distance = nearestSatelliteNorad->getDistance(gsInfo.mobility->getLUTPositionY(), gsInfo.mobility->getLUTPositionX(), 0);
            // delay of the channel between nearest satellite and groundstation (ms)
            double delay = distance * groundlinkDelay;

            // has nearest satellite and is the old one
            if (gsInfo.satellite == nearestSatellite)
            {
                // update the channel
                updateOrCreateChannel(uplinkO, downlinkI, delay, groundlinkDatarate);
                updateOrCreateChannel(downlinkO, uplinkI, delay, groundlinkDatarate);
                EV << "Groundstation update: " << gsInfo.to_string() << endl;
            }
            // has nearest satellite and is not the old one
            else
            {
                // remove connection between old nearest satellite and groundstation
                if (gsInfo.satellite != nullptr)
                {
                    cGate *oldUplinkO = gsInfo.satellite->gateHalf("groundLink", cGate::Type::OUTPUT);
                    oldUplinkO->disconnect();
                }
                deleteChannel(uplinkO);
                deleteChannel(downlinkO);

                // connect new
                updateOrCreateChannel(uplinkO, downlinkI, delay, groundlinkDatarate);
                updateOrCreateChannel(downlinkO, uplinkI, delay, groundlinkDatarate);
                gsInfo.satellite = nearestSatellite;
                EV << "Groundstation connect [Distance: " << distance << "; Delay: " << delay << "]: " << gsInfo.to_string() << endl;
                topologyChanged = true;
            }
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
    }

    void TopologyControl::updateISLInWalkerStar()
    {
        int satCount = satellites.size();
        for (size_t i = 0; i < satCount; i++)
        {
            std::pair<cModule *, NoradA *> curSat = satellites.at(i);
            int satPlane = calculateSatellitePlane(i);

            bool isLastPlane = satPlane == planeCount - 1;
            if (isLastPlane)
                break;
            int rightIndex = (i + satsPerPlane) % satCount;
            std::pair<cModule *, NoradA *> nextPlaneSat = satellites.at(rightIndex);

            // calculate ISL channel params
            double distance = curSat.second->getDistance(nextPlaneSat.second->getLatitude(), nextPlaneSat.second->getLongitude(), nextPlaneSat.second->getAltitude());
            double delay = islDelay * distance;

            if (curSat.second->isAscending())
            { // sat is moving up
                cGate *rightGateOut = curSat.first->gateHalf("right", cGate::Type::OUTPUT);
                cGate *leftGateOut = nextPlaneSat.first->gateHalf("left", cGate::Type::OUTPUT);

                if (nextPlaneSat.second->isAscending() && isIslEnabled(curSat.second->getLatitude()) && isIslEnabled(nextPlaneSat.second->getLatitude()))
                { // they are allowed to connect
                    cGate *rightGateIn = curSat.first->gateHalf("right", cGate::Type::INPUT);
                    cGate *leftGateIn = nextPlaneSat.first->gateHalf("left", cGate::Type::INPUT);

                    // generate or update ISL channel from right to left sat
                    ChannelState state1 = updateOrCreateChannel(rightGateOut, leftGateIn, delay, islDatarate);
                    ChannelState state2 = updateOrCreateChannel(leftGateOut, rightGateIn, delay, islDatarate);

                    // if any channel was created, we have a topologyupdate
                    if (state1 == ChannelState::CREATED || state2 == ChannelState::CREATED)
                    {
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
                        topologyChanged = true;
                    }
                }
            }
            else
            { // sat is moving down
                cGate *leftGateOut = curSat.first->gateHalf("left", cGate::Type::OUTPUT);
                cGate *rightGateOut = nextPlaneSat.first->gateHalf("right", cGate::Type::OUTPUT);

                if (!nextPlaneSat.second->isAscending() && isIslEnabled(curSat.second->getLatitude()) && isIslEnabled(nextPlaneSat.second->getLatitude()))
                { // they are allowed to connect
                    cGate *leftGateIn = curSat.first->gateHalf("left", cGate::Type::INPUT);
                    cGate *rightGateIn = nextPlaneSat.first->gateHalf("right", cGate::Type::INPUT);

                    // generate or update ISL channel from right to left sat
                    ChannelState state1 = updateOrCreateChannel(leftGateOut, rightGateIn, delay, islDatarate);
                    ChannelState state2 = updateOrCreateChannel(rightGateOut, leftGateIn, delay, islDatarate);

                    // if any channel was created, we have a topologyupdate
                    if (state1 == ChannelState::CREATED || state2 == ChannelState::CREATED)
                    {
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
            cDatarateChannel *channel = cDatarateChannel::create("IslChannel");
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
    }

} // flora
