/*
 * SatelliteInfo.h
 *
 * Created on: Feb 10, 2023
 *     Author: Robin Ohs
 */

#ifndef __FLORA_TOPOLOGYCONTROL_SATELLITEINFO_H_
#define __FLORA_TOPOLOGYCONTROL_SATELLITEINFO_H_

#include <omnetpp.h>

#include <sstream>

#include "PositionAwareBase.h"
#include "core/Constants.h"
#include "core/ISLDirection.h"
#include "mobility/NoradA.h"

using namespace omnetpp;
using flora::core::Constants;
using flora::core::isldirection::Direction;

namespace flora {
namespace topologycontrol {

class SatelliteInfo : public PositionAwareBase {
   private:
    int satelliteId;
    cModule *satelliteModule;

   public:
    NoradA *noradModule;
    int leftSatellite = -1;
    int rightSatellite = -1;
    int upSatellite = -1;
    int downSatellite = -1;

   public:
    SatelliteInfo(int satId, cModule *satModule, NoradA *norad) : satelliteId(satId),
                                                                  satelliteModule(satModule),
                                                                  noradModule(norad){};

    int getSatelliteId() const { return satelliteId; }

    cGate *getInputGate(Direction direction, int index = -1) const;
    cGate *getOutputGate(Direction direction, int index = -1) const;

    double getLatitude() const override;
    double getLongitude() const override;
    double getAltitude() const override;

    /** Returns the elevation from this entity to a reference entity. */
    double getElevation(const PositionAwareBase &other) const;
    /** Returns the azimuth from this entity to a reference entity. */
    double getAzimuth(const PositionAwareBase &other) const;
    /** Returns whether the satellite is currently ascending. */
    bool isAscending() const;
    /** Returns whether the satellite is currently descending. */
    bool isDescending() const;

    friend std::ostream &operator<<(std::ostream &ss, const SatelliteInfo &p) {
        ss << "{";
        ss << "\"satelliteId\": " << p.satelliteId << ",";
        ss << "\"up\": " << p.upSatellite << ",";
        ss << "\"down\": " << p.downSatellite << ",";
        ss << "\"left\": " << p.leftSatellite << ",";
        ss << "\"right\": " << p.rightSatellite << ",";
        ss << "}";
        return ss;
    }
};

}  // namespace topologycontrol
}  // namespace flora

#endif  // __FLORA_TOPOLOGYCONTROL_SATELLITEINFO_H_