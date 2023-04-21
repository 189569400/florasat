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
#include <string>

#include "PositionAwareBase.h"
#include "core/Constants.h"
#include "core/ISLDirection.h"
#include "core/Utils.h"
#include "mobility/NoradA.h"

using namespace omnetpp;
using namespace flora::core;
using flora::core::Constants;

namespace flora {
namespace topologycontrol {

class SatelliteInfo : public PositionAwareBase {
   private:
    int satelliteId;
    cModule *satelliteModule;
    NoradA *noradModule;
    double leftDistance = -1;
    double rightDistance = -1;
    double upDistance = -1;
    double downDistance = -1;
    int leftSatellite = -1;
    int rightSatellite = -1;
    int upSatellite = -1;
    int downSatellite = -1;

   public:
    SatelliteInfo(int satId, cModule *satModule, NoradA *norad) : satelliteId(satId),
                                                                  satelliteModule(satModule),
                                                                  noradModule(norad){};

    int getSatelliteId() const { return satelliteId; }

    cGate *getInputGate(isldirection::Direction direction, int index = -1) const;
    cGate *getOutputGate(isldirection::Direction direction, int index = -1) const;

    void setLeftSat(const SatelliteInfo &newLeft);
    void setUpSat(const SatelliteInfo &newUp);
    void setRightSat(const SatelliteInfo &newRight);
    void setDownSat(const SatelliteInfo &newDown);

    void removeLeftSat();
    void removeUpSat();
    void removeRightSat();
    void removeDownSat();

    bool hasLeftSat() const;
    bool hasUpSat() const;
    bool hasRightSat() const;
    bool hasDownSat() const;

    int getLeftSat() const;
    int getUpSat() const;
    int getRightSat() const;
    int getDownSat() const;

    double getLeftSatDistance() const;
    double getUpSatDistance() const;
    double getRightSatDistance() const;
    double getDownSatDistance() const;

    double getLatitude() const override;
    double getLongitude() const override;
    double getAltitude() const override;

    /** @brief Returns the plane of the satellite. */
    int getPlane() const;
    /** @brief Gives the number in the plane. The first sat in any plane has number 0.*/
    int getNumberInPlane() const;
    /** @brief Returns the elevation from this entity to a reference entity. */
    double getElevation(const PositionAwareBase &other) const;
    /** @brief Returns the azimuth from this entity to a reference entity. */
    double getAzimuth(const PositionAwareBase &other) const;
    /** @brief Returns whether the satellite is currently ascending. */
    bool isAscending() const;
    /** @brief Returns whether the satellite is currently descending. */
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