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
            return;
        }
        EV << "initialize TopologyControl" << endl;
        updateInterval = par("updateInterval");
        walkerType = WalkerType::parseWalkerType(par("walkerType"));
        isClosedConstellation = par("isClosedConstellation");
        lowerLatitudeBound = par("lowerLatitudeBound");
        upperLatitudeBound = par("upperLatitudeBound");
        islDelay = par("islDelay");
        islDatarate = par("islDatarate");
        minimumElevation = par("minimumElevation");
        EV << "Load parameters: "
           << "updateInterval: " << updateInterval << "; "
           << "isClosedConstellation: " << isClosedConstellation << "; "
           << "lowerLatitudeBound: " << lowerLatitudeBound << "; "
           << "upperLatitudeBound: " << upperLatitudeBound << "; "
           << "islDelay: " << islDelay << "; "
           << "islDatarate: " << islDatarate << "; "
           << "minimumElevation: " << minimumElevation << endl;

        satellites = getSatellites();
        groundstationSatellites = getGroundstations();

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
        // take first satellite and read number of planes + satellitesPerPlane
        planeCount = satellites.at(0).second->getNumberOfPlanes();
        satsPerPlane = satellites.at(0).second->getSatellitesPerPlane();

        // update ISL links
        updateIntraSatelliteLinks();
        updateInterSatelliteLinks();
        updateGroundstationLinks();
    }

    std::map<cModule *, std::vector<int>> TopologyControl::getGroundstations()
    {
        std::map<cModule *, std::vector<int>> loadedGroundstations;
        int gsCount = getSystemModule()->getSubmoduleVectorSize("groundStation");
        for (size_t i = 0; i < gsCount; i++)
        {
            cModule *groundstation = getSystemModule()->getSubmodule("groundStation", i);
            if (groundstation == nullptr)
            {
                error("Error in TopologyControl::getGroundstations(): groundStation with index %zu is nullptr. Make sure the module exists.", i);
            }
            std::vector<int> emptyvector = {};
            loadedGroundstations.emplace(groundstation, emptyvector);
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
                double delay = (islDelay * distance) / 1000000.0;

                // generate or update ISL channel from lower to upper sat
                if (fromGateOut->isConnectedOutside())
                {
                    cDatarateChannel *isl_channel_up = check_and_cast<cDatarateChannel *>(fromGateOut->getChannel());
                    isl_channel_up->setDelay(delay);
                    isl_channel_up->setDatarate(islDatarate);
                }
                else
                {
                    EV << "Connect " << index << " to " << (isLastSatInPlane ? plane * satsPerPlane : index + 1) << "; distance: " << distance << "; delay: " << delay << ";" << endl;
                    cDatarateChannel *isl_channel_up = cDatarateChannel::create("IslChannel");
                    isl_channel_up->setDelay(delay);
                    isl_channel_up->setDatarate(islDatarate);
                    fromGateOut->connectTo(toGateIn, isl_channel_up);
                    isl_channel_up->callInitialize();
                }

                // generate or update ISL channel from upper to lower sat
                if (toGateOut->isConnectedOutside())
                {
                    cDatarateChannel *isl_channel_down = check_and_cast<cDatarateChannel *>(toGateOut->getChannel());
                    isl_channel_down->setDelay(delay);
                    isl_channel_down->setDatarate(islDatarate);
                }
                else
                {
                    EV << "Connect " << index << " to " << (isLastSatInPlane ? plane * satsPerPlane : index + 1) << "; distance: " << distance << "; delay: " << delay << ";" << endl;
                    cDatarateChannel *isl_channel_down = cDatarateChannel::create("IslChannel");
                    isl_channel_down->setDelay(delay);
                    isl_channel_down->setDatarate(islDatarate);
                    toGateOut->connectTo(fromGateIn, isl_channel_down);
                    isl_channel_down->callInitialize();
                }
            }
        }
    }

    void TopologyControl::updateGroundstationLinks()
    {
        auto iter = groundstationSatellites.begin();
        while (iter != groundstationSatellites.end())
        {
            cModule *groundStation = iter->first;
            GroundStationMobility *mobility = check_and_cast<GroundStationMobility *>(groundStation->getSubmodule("mobility"));

            if (mobility == nullptr) {
                error("Error in TopologyControl::updateGroundstationLinks(): mobility module of Groundstation is nullptr. Make sure a module with name `mobility` exists.");
            }

            std::vector<int> satellitesInRange;
            for (size_t i = 0; i < satellites.size(); i++)
            {
                double elevation = ((INorad *)satellites.at(i).second)->getElevation(mobility->getLUTPositionY(), mobility->getLUTPositionX());
                if (elevation >= minimumElevation)
                {
                    satellitesInRange.push_back(i);
                }
            }
            groundstationSatellites.emplace();
            ++iter;
        };
        utilities::PrintMap(groundstationSatellites);
    }

    void TopologyControl::updateInterSatelliteLinks()
    {
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
            double delay = (islDelay * distance) / 1000000.0;
            if (curSat.second->isAscending())
            { // sat is moving up
                cGate *rightGateOut = curSat.first->gateHalf("right", cGate::Type::OUTPUT);
                cGate *leftGateOut = nextPlaneSat.first->gateHalf("left", cGate::Type::OUTPUT);

                if (nextPlaneSat.second->isAscending() && isIslEnabled(curSat.second->getLatitude()) && isIslEnabled(nextPlaneSat.second->getLatitude()))
                { // they are allowed to connect
                    cGate *rightGateIn = curSat.first->gateHalf("right", cGate::Type::INPUT);
                    cGate *leftGateIn = nextPlaneSat.first->gateHalf("left", cGate::Type::INPUT);

                    // generate or update ISL channel from right to left sat
                    updateOrCreateChannel(rightGateOut, leftGateIn, delay);
                    updateOrCreateChannel(leftGateOut, rightGateIn, delay);
                }
                else
                { // they are not allowed to have an connection
                    rightGateOut->disconnect();
                    leftGateOut->disconnect();
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
                    updateOrCreateChannel(leftGateOut, rightGateIn, delay);
                    updateOrCreateChannel(rightGateOut, leftGateIn, delay);
                }
                else
                { // they are not allowed to have an connection
                    leftGateOut->disconnect();
                    rightGateOut->disconnect();
                }
            }
        }
    }

    bool TopologyControl::isIslEnabled(double latitude)
    {
        return latitude <= upperLatitudeBound && latitude >= lowerLatitudeBound;
    }

    void TopologyControl::updateOrCreateChannel(cGate *outGate, cGate *inGate, double delay)
    {
        if (outGate->isConnectedOutside())
        {
            cDatarateChannel *channel = check_and_cast<cDatarateChannel *>(outGate->getChannel());
            channel->setDelay(delay);
            channel->setDatarate(islDatarate);
        }
        else
        {
            cDatarateChannel *channel = cDatarateChannel::create("IslChannel");
            channel->setDelay(delay);
            channel->setDatarate(islDatarate);
            outGate->connectTo(inGate, channel);
            channel->callInitialize();
        }
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

} // flora