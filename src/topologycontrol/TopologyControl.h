/*
 * TopologyControl.h
 *
 * Created on: Dec 20, 2022
 *     Author: Robin Ohs
 */

#ifndef TOPOLOGYCONTROL_TOPOLOGYCONTROL_H
#define TOPOLOGYCONTROL_TOPOLOGYCONTROL_H

#include <string.h>
#include <omnetpp.h>
#include "topologycontrol/utilities/WalkerType.h"
#include "mobility/NoradA.h"

using namespace omnetpp;

namespace flora
{

    class TopologyControl : public cSimpleModule
    {
    public:
        TopologyControl();
        void UpdateTopology();

    protected:
        virtual ~TopologyControl();
        virtual void initialize(int stage) override;
        virtual int numInitStages() const override { return 2; }
        virtual void handleMessage(cMessage *msg) override;
        void handleSelfMessage(cMessage *msg);
        /** @brief Schedules the update timer that will update the topology state.*/
        void scheduleUpdate();

    private:
        std::map<int, std::pair<cModule *, NoradA *>> getSatellites();
        WalkerType parseWalkerType(std::string value);
        void updateIntraSatelliteLinks(std::map<int, std::pair<cModule *, NoradA *>> satellites, int planeCount, int satsPerPlane);
        void updateInterSatelliteLinks(std::map<int, std::pair<cModule *, NoradA *>> satellites, int planeCount, int satsPerPlane);
        void updateISLInWalkerDelta(std::map<int, std::pair<cModule *, NoradA *>> satellites, int planeCount, int satsPerPlane);
        void updateISLInWalkerStar(std::map<int, std::pair<cModule *, NoradA *>> satellites, int planeCount, int satsPerPlane);
        bool isIslEnabled(double latitude);
        void updateOrCreateChannel(cGate *outGate, cGate *inGate, double delay);
        int calculateSatellitePlane(int id, int planeCount, int satsPerPlane);

    protected:
        /** @brief The message used for TopologyControl state changes. */
        cMessage *updateTimer;

        /** @brief The upper latitude to shut down inter-plane ISL. (Noth-Pole) */
        double upperLatitudeBound;

        /** @brief The lower latitude to shut down inter-plane ISL. (South-Pole) */
        double lowerLatitudeBound;

        /**
         * @brief The simulation time interval used to regularly signal mobility state changes.
         *
         * The 0 value turns off the signal.
         */
        simtime_t updateInterval;

        /** @brief Are satellites distributed over entire plane (Connect last and first satellite in plane). */
        bool isClosedConstellation;

        /** @brief Delay of the isl channel, in microseconds/km. */
        double islDelay;

        /** @brief Datarate of the isl channel, in bit/second. */
        double islDatarate;

        /** @brief The minimum elevation between a satellite and a groundstation.*/
        double minimumElevation;

        /** @brief The type of the walker constellation.*/
        WalkerType walkerType;
    };

} // flora

#endif // TOPOLOGYCONTROL_TOPOLOGYCONTROL_H
