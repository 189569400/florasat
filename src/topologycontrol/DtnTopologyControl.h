/*
 * DtnTopologyControl.h
 *
 * Created on: May 17, 2023
 *     Author: Sebastian Montoya
 */

#ifndef __FLORA_TOPOLOGYCONTROL_DTNTOPOLOGYCONTROL_H_
#define __FLORA_TOPOLOGYCONTROL_DTNTOPOLOGYCONTROL_H_

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
#include "routing/dtn/contactplan/ContactPlan.h"
#include "topologycontrol/TopologyControlBase.h"


using namespace omnetpp;
using namespace flora::satellite;
using namespace flora::core;

namespace flora {
namespace topologycontrol {

class DtnTopologyControl : public TopologyControlBase {
   protected:
    virtual void initialize(int stage) override;
    virtual void updateTopology() override;

   private:
    void updateIntraSatelliteLinks();
    void updateInterSatelliteLinks();
    void updateGroundstationLinksDtn();
    void updateISLInWalkerDelta();
    void updateISLInWalkerStar();
    void linkGroundStationToSatDtn(int gsId, int satId);
    void unlinkGroundStationToSatDtn(int gsId, int satId);
    void updateLinkGroundStationToSatDtn(int gsId, int satId);
    bool isDtnContactStarting(int gsId, int satId, Contact contact);
    bool isDtnContactTakingPlace(int gsId, int satId, Contact contact);
    bool isDtnContactEnding(int gsId, int satId, Contact contact);
};

}  // namespace topologycontrol
}  // namespace flora

#endif  // __FLORA_TOPOLOGYCONTROL_DTNTOPOLOGYCONTROL_H_