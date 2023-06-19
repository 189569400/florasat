/*
 * DtnTopologyControl.h
 *
 * Created on: Jun 16, 2023
 *     Author: Sebastian Montoya
 */

#ifndef __FLORA_TOPOLOGYCONTROL_CONTACTPLANTOPOLOGYCONTROL_H_
#define __FLORA_TOPOLOGYCONTROL_CONTACTPLANTOPOLOGYCONTROL_H_

#include "core/Timer.h"
#include "routing/dtn/contactplan/ContactPlan.h"
#include "topologycontrol/TopologyControlBase.h"

using namespace omnetpp;
using namespace flora::satellite;
using namespace flora::core;

namespace flora {
namespace topologycontrol {

class ContactPlanTopologyControl : public TopologyControlBase {

   protected:
    virtual void initialize(int stage) override;
    virtual void updateTopology() override;

    /** @brief Structs that represent connections between nodes, groundstations or satellites */
    std::map<std::pair<int, int>, double> contactStarts;

   private:
    void updateIntraSatelliteLinks();
    void updateInterSatelliteLinks();
    void updateGroundstationLinksDtn();
    int getShiftedSatelliteId(int satId);
    string distanceMode;
    bool isGroundStationContactValid(GroundStationRoutingBase *gs, SatelliteRoutingBase *sat);
};

}  // namespace topologycontrol
}  // namespace flora

#endif  // __FLORA_TOPOLOGYCONTROL_CONTACTPLANTOPOLOGYCONTROL_H_
