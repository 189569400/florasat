/*
 * ConstellationTopologyControl.h
 *
 * Created on: Dec 20, 2022
 *     Author: Robin Ohs
 */

#ifndef __FLORA_TOPOLOGYCONTROL_CONSTELLATIONTOPOLOGYCONTROL_H_
#define __FLORA_TOPOLOGYCONTROL_CONSTELLATIONTOPOLOGYCONTROL_H_

#include <omnetpp.h>

#include <algorithm>
#include <set>
#include <string>
#include <unordered_map>

#include "core/Constants.h"
#include "core/Timer.h"
#include "core/Utils.h"
#include "core/WalkerType.h"
#include "core/utils/SetUtils.h"
#include "satellite/SatelliteRoutingBase.h"
#include "inet/common/clock/ClockUserModuleMixin.h"
#include "mobility/GroundStationMobility.h"
#include "mobility/NoradA.h"
#include "topologycontrol/data/GroundstationInfo.h"
#include "topologycontrol/data/GsSatConnection.h"
#include "topologycontrol/utilities/ChannelState.h"
#include "topologycontrol/utilities/PrintMap.h"
#include "TopologyControlBase.h"

using namespace omnetpp;
using namespace flora::satellite;
using namespace flora::core;

namespace flora {
namespace topologycontrol {

class ConstellationTopologyControl : public TopologyControlBase {
   protected:
    virtual void initialize(int stage) override;
    virtual void updateTopology() override;

   private:
    void updateIntraSatelliteLinks();
    void updateInterSatelliteLinks();
    void updateGroundstationLinks();
    void updateISLInWalkerDelta();
    void updateISLInWalkerStar();

};

}  // namespace topologycontrol
}  // namespace flora

#endif  // __FLORA_TOPOLOGYCONTROL_CONSTELLATIONTOPOLOGYCONTROL_H_
