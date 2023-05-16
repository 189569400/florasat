/*
 * GroundstationInfo.h
 *
 * Created on: Feb 06, 2023
 *     Author: Robin Ohs
 */

#ifndef __FLORA_TOPOLOGYCONTROL_GROUNDSTATIONINFO_H_
#define __FLORA_TOPOLOGYCONTROL_GROUNDSTATIONINFO_H_

#include <omnetpp.h>

#include <set>
#include <sstream>

#include "core/Constants.h"
#include "core/PositionAwareBase.h"
#include "mobility/GroundStationMobility.h"

using namespace omnetpp;

namespace flora {
namespace topologycontrol {

class GroundstationInfo: public flora::core::PositionAwareBase {
   private:
    int groundStationId;
    cModule *groundStation;

   public:
    std::set<int> satellites;
    GroundStationMobility *mobility;

   public:
    GroundstationInfo(int gsId, cModule *gs, GroundStationMobility *gsMobility) : groundStationId(gsId),
                                                                                  groundStation(gs),
                                                                                  mobility(gsMobility){};
    int getGroundStationId() const { return groundStationId; }

    cGate *getInputGate(int index) const;
    cGate *getOutputGate(int index) const;

    double getLatitude() const override;
    double getLongitude() const override;
    double getAltitude() const override;

    friend std::ostream &operator<<(std::ostream &ss, const GroundstationInfo &gs) {
        ss << "{";
        ss << "\"groundStationId\": " << gs.groundStationId << ",";
        ss << "\"groundStation\": " << gs.groundStation << ",";
        ss << "\"satellites\": "
           << "[";
        for (int satellite : gs.satellites) {
            ss << satellite << ",";
        }
        ss << "],";
        ss << "}";
        return ss;
    }
};

}  // namespace topologycontrol
}  // namespace flora

#endif  // __FLORA_TOPOLOGYCONTROL_GROUNDSTATIONINFO_H_