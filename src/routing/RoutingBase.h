/*
 * RoutingBase.h
 *
 * Created on: Feb 04, 2023
 *     Author: Robin Ohs
 */

#ifndef __FLORA_ROUTING_ROUTINGBASE_H_
#define __FLORA_ROUTING_ROUTINGBASE_H_

#include <omnetpp.h>

#include <vector>

#include "RoutingHeader_m.h"
#include "core/utils/VectorUtils.h"
#include "inet/common/packet/Packet.h"
#include "routing/core/DijkstraShortestPath.h"
#include "satellite/SatelliteRouting.h"
#include "topologycontrol/TopologyControlBase.h"

namespace flora {
namespace routing {

using namespace omnetpp;
using namespace inet;
using namespace isldirection;

class RoutingBase : public cSimpleModule {
   protected:
    topologycontrol::TopologyControlBase *topologyControl = nullptr;

   public:
    /**
     * Handler that is called on all preplannable topology changes.
     * E.g., is not called if ISL connection does no longer work.
     * Can be used to simulate preplanned data.
     *
     * -> Base implementation is intentionally blank.
     */
    virtual void handleTopologyChange(){};

    /**
     * Method to calculate suitable first and last satellites based on DSPA.
     */
    virtual std::pair<int, int> calculateFirstAndLastSatellite(int srcGs, int dstGs);

    /**
     * Most important method for routing modules, must be implemented by all subclasses.
     * Given a pointer to a RoutingHeader and the caller satellite, decides the next direction.
     */
    virtual ISLDirection routePacket(inet::Ptr<const RoutingHeader> rh, SatelliteRouting *callerSat) = 0;

    /**
     * Handler for messages exchanged between satellites to organize congestion controll, etc.
     *
     * -> Base implementation is intentionally blank, as not all algorithms are dynamic.
     */
    virtual void handleMessage(inet::Packet *pkt, SatelliteRouting *callerSat){};

    /**
     * Handler for omnetpp self messages.
     * 
     * -> Base implementation is intentionally blank, as not all algorithms require self messages.
    */
    virtual void handleMessage(cMessage *msg) override {
        if(!msg->isSelfMessage()) {
            error("Only accepts self messages!");
        }
    };

    /**
     * Handler called if the packet handler dropps a packet.
     * Returns a pointer to a packet that is broadcasted afterwards.
     * Will do nothing if nullptr is returned.
     *
     * -> Base implementation is intentionally blank, as not all algorithms are dynamic.
     */
    virtual inet::Packet *handlePacketDrop(const inet::Packet *pkt, SatelliteRouting *callerSat, inet::PacketDropReason reason) { return nullptr; };

    /**
     * Handler called if satellites enqueues a packet. Is called with the current queueSize.
     * Returns a pointer to a packet that is broadcasted afterwards.
     * Will do nothing if nullptr is returned.
     *
     * -> Base implementation is intentionally blank, as not all algorithms are dynamic.
     */
    virtual inet::Packet *handleQueueSize(SatelliteRouting *callerSat, int queueSize, int maxQueueSize) { return nullptr; };

    /** @brief Returns the index of a groundlink inside the groundlink gate vector.*/
    int getGroundlinkIndex(int satelliteId, int groundstationId);

   protected:
    virtual void initialize(int stage) override;
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    std::set<int> const &getConnectedSatellites(int groundStationId) const;
};

}  // namespace routing
}  // namespace flora

#endif  // __FLORA_ROUTING_ROUTINGBASE_H_
