/*
 * ConstellationTopologyControl.h
 *
 * Created on: Dec 20, 2022
 *     Author: Robin Ohs
 */

#ifndef __FLORA_TOPOLOGYCONTROL_CONSTELLATIONTOPOLOGYCONTROL_H_
#define __FLORA_TOPOLOGYCONTROL_CONSTELLATIONTOPOLOGYCONTROL_H_

#include "TopologyControlBase.h"
#include "core/Timer.h"
#include "routing/RoutingBase.h"

using namespace omnetpp;
using namespace flora::satellite;
using namespace flora::core;

namespace flora {

namespace topologycontrol {

class ConstellationTopologyControl : public TopologyControlBase {
   protected:
    virtual void initialize(int stage) override;
    virtual void updateTopology() override;
    virtual void trackTopologyChange() override;

   private:
    void updateIntraSatelliteLinks();
    void updateInterSatelliteLinks();
    void updateGroundstationLinks();
    void updateISLInWalkerDelta();
    void updateISLInWalkerStar();

   protected:
    routing::RoutingBase* routing;
};

}  // namespace topologycontrol
}  // namespace flora

#endif  // __FLORA_TOPOLOGYCONTROL_CONSTELLATIONTOPOLOGYCONTROL_H_
