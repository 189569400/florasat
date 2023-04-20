/*
 * TopologyControl.h
 *
 * Created on: Dec 20, 2022
 *     Author: Robin Ohs
 */

#ifndef __FLORA_TOPOLOGYCONTROL_TOPOLOGYCONTROL_H_
#define __FLORA_TOPOLOGYCONTROL_TOPOLOGYCONTROL_H_

#include <omnetpp.h>

#include <algorithm>
#include <set>
#include <string>
#include <unordered_map>

#include "core/Constants.h"
#include "core/Timer.h"
#include "core/Utils.h"
#include "core/utils/SetUtils.h"
#include "inet/common/clock/ClockUserModuleMixin.h"
#include "mobility/GroundStationMobility.h"
#include "mobility/NoradA.h"
#include "topologycontrol/data/GroundstationInfo.h"
#include "topologycontrol/data/GsSatConnection.h"
#include "topologycontrol/data/SatelliteInfo.h"
#include "topologycontrol/utilities/ChannelState.h"
#include "topologycontrol/utilities/PrintMap.h"
#include "topologycontrol/utilities/WalkerType.h"

using namespace omnetpp;
using namespace flora::core;

namespace flora {
namespace topologycontrol {

class TopologyControl : public ClockUserModuleMixin<cSimpleModule> {
   public:
    TopologyControl();
    void updateTopology();
    GroundstationInfo const &getGroundstationInfo(int gsId) const;
    SatelliteInfo const &getSatelliteInfo(int satId) const;
    GsSatConnection const &getGroundstationSatConnection(int gsId, int satId) const;

   protected:
    virtual ~TopologyControl();
    virtual void initialize(int stage) override;
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    virtual void handleMessage(cMessage *msg) override;
    void handleSelfMessage(cMessage *msg);
    /** @brief Schedules the update timer that will update the topology state.*/
    void scheduleUpdate();

   private:
    void loadSatellites();
    void loadGroundstations();
    void updateIntraSatelliteLinks();
    void updateInterSatelliteLinks();
    void updateGroundstationLinks();
    void updateISLInWalkerDelta();
    void updateISLInWalkerStar();
    void trackTopologyChange();
    SatelliteInfo &findSatByPlaneAndNumberInPlane(int plane, int numberInPlane);
    bool isIslEnabled(PositionAwareBase &entity) const;

    void connectSatellites(SatelliteInfo &first, SatelliteInfo &second, isldirection::Direction firstOutDirection);
    void disconnectSatellites(SatelliteInfo &first, SatelliteInfo &second, isldirection::Direction firstOutDirection);
    void removeOldConnections(SatelliteInfo &first, SatelliteInfo &second, isldirection::Direction direction);
    /** @brief Creates/Updates the channel from outGate to inGate. If channel exists updates channel params, otherwise creates the channel.*/
    ChannelState updateOrCreateChannel(cGate *outGate, cGate *inGate, double delay, double datarate);
    /** @brief Deletes the channel of outGate. If channel does not exist, nothing happens, otherwise deletes the channel.*/
    ChannelState deleteChannel(cGate *outGate);
    /** @brief Takes a ISL gate and deletes the connection from this to another ISL gate and the way back. */
    void disconnectGate(cGate *gate);

   protected:
    /**
     * @brief The simulation time interval used to regularly signal mobility state changes.
     *
     * The 0 value turns off the signal.
     */
    cPar *updateIntervalParameter = nullptr;
    ClockEvent *updateTimer = nullptr;

    /** @brief Map of satellite ids and their correspinding SatelliteInfo data struct. */
    std::unordered_map<int, SatelliteInfo> satelliteInfos;
    int numSatellites;

    /** @brief Structs that represent groundstations and all satellites in range. */
    std::unordered_map<int, GroundstationInfo> groundstationInfos;
    int numGroundStations;
    int numGroundLinks = 40;

    /** @brief Structs that represent connections between satellites and groundstations. */
    std::map<std::pair<int, int>, GsSatConnection> gsSatConnections;

    /** @brief Can be set to true for constellations that do not feature inter-plane ISL*/
    bool interPlaneIslDisabled;
    /** @brief The upper latitude to shut down inter-plane ISL. (Noth-Pole) */
    double upperLatitudeBound;
    /** @brief The lower latitude to shut down inter-plane ISL. (South-Pole) */
    double lowerLatitudeBound;
    /** @brief Delay of the isl channel, in microseconds/km. */
    double islDelay;
    /** @brief Datarate of the isl channel, in bit/second. */
    double islDatarate;

    /** @brief Delay of the groundlink channel, in microseconds/km. */
    double groundlinkDelay;
    /** @brief Datarate of the groundlink channel, in bit/second. */
    double groundlinkDatarate;
    /** @brief The minimum elevation between a satellite and a groundstation.*/
    double minimumElevation;

    WalkerType::WalkerType walkerType;
    int interPlaneSpacing;
    int planeCount;
    int satsPerPlane;

    /** @brief Used to indicate if there was a change to the topology. */
    bool topologyChanged = false;
};

}  // namespace topologycontrol
}  // namespace flora

#endif  // __FLORA_TOPOLOGYCONTROL_TOPOLOGYCONTROL_H_
