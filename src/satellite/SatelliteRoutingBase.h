/*
 * SatelliteRoutingBase.h
 *
 *  Created on: May 15, 2023
 *      Author: Robin Ohs
 */

#ifndef __FLORA_SATELLITE_SATELLITEROUTINGBASE_H_
#define __FLORA_SATELLITE_SATELLITEROUTINGBASE_H_

#include <omnetpp.h>

#include "core/Constants.h"
#include "core/ISLDirection.h"
#include "core/Utils.h"
#include "inet/common/INETDefs.h"
#include "inet/common/Simsignals.h"
#include "inet/common/packet/Packet.h"
#include "mobility/NoradA.h"
#include "routing/RoutingHeader_m.h"
#include "core/PositionAwareBase.h"

using namespace omnetpp;
using namespace inet;
using namespace flora::core;
using flora::core::Constants;

namespace flora {
namespace satellite {

class SatelliteRoutingBase : public cSimpleModule, public PositionAwareBase {
   private:
    int satId;
    int satPlane;
    int satNumberInPlane;
    NoradA *noradModule;
    /** @brief The upper latitude to shut down inter-plane ISL. (Noth-Pole) */
    double upperLatitudeBound;
    /** @brief The lower latitude to shut down inter-plane ISL. (South-Pole) */
    double lowerLatitudeBound;

    SatelliteRoutingBase *leftSatellite = nullptr;
    SatelliteRoutingBase *rightSatellite = nullptr;
    SatelliteRoutingBase *upSatellite = nullptr;
    SatelliteRoutingBase *downSatellite = nullptr;

   public:
    /** @brief Returns the id of the satellite. */
    int getId() const { return satId; }
    /** @brief Returns the plane of the satellite. */
    int getPlane() const { return satPlane; }
    /** @brief Gives the number in the plane. The first sat in any plane has number 0.*/
    int getNumberInPlane() const { return satNumberInPlane; }

    cGate *getInputGate(isldirection::Direction direction, int index = -1);
    cGate *getOutputGate(isldirection::Direction direction, int index = -1);

    /** @brief Connects the satellite to the other satellite. The return value indicates whether the connection is new or the channel params were updated.*/
    bool connect(SatelliteRoutingBase *other, isldirection::Direction direction);

    /** @brief Disconnects the satellite on the given direction. The return value indicates whether the connection was deleted or did not exist.*/
    bool disconnect(isldirection::Direction direction);
    //  /** @brief Connects the satellite via right Inter-Plane ISL to the other satellite. The return value indicates whether the connection is new or the channel params were updated.*/
    //  bool connectRight(SatelliteRoutingBase *other);

    bool hasLeftSat() const;
    bool hasUpSat() const;
    bool hasRightSat() const;
    bool hasDownSat() const;

    SatelliteRoutingBase *getLeftSat() const;
    SatelliteRoutingBase *getUpSat() const;
    SatelliteRoutingBase *getRightSat() const;
    SatelliteRoutingBase *getDownSat() const;

    void setLeftSat(SatelliteRoutingBase *newSat);
    void setUpSat(SatelliteRoutingBase *newSat);
    void setRightSat(SatelliteRoutingBase *newSat);
    void setDownSat(SatelliteRoutingBase *newSat);

    void removeLeftSat();
    void removeUpSat();
    void removeRightSat();
    void removeDownSat();

    int getLeftSatId() const;
    int getUpSatId() const;
    int getRightSatId() const;
    int getDownSatId() const;

    double getLeftSatDistance() const;
    double getUpSatDistance() const;
    double getRightSatDistance() const;
    double getDownSatDistance() const;

    double getLatitude() const override;
    double getLongitude() const override;
    double getAltitude() const override;

    /** @brief Returns the elevation from this entity to a reference entity. */
    double getElevation(const PositionAwareBase &other) const;
    /** @brief Returns the azimuth from this entity to a reference entity. */
    double getAzimuth(const PositionAwareBase &other) const;
    /** @brief Returns whether the satellite is currently ascending. */
    bool isAscending() const;
    /** @brief Returns whether the satellite is currently descending. */
    bool isDescending() const;

    /** @brief Returns whether the satellite is currently able to create inter plane ISL connections. */
    bool isInterPlaneISLEnabled() const;

    friend std::ostream &operator<<(std::ostream &ss, const SatelliteRoutingBase &p) {
        ss << "{";
        ss << "\"satelliteId\": " << p.satId << ",";
        ss << "\"up\": " << p.upSatellite << ",";
        ss << "\"down\": " << p.downSatellite << ",";
        ss << "\"left\": " << p.leftSatellite << ",";
        ss << "\"right\": " << p.rightSatellite << ",";
        ss << "}";
        return ss;
    }

   protected:
    virtual void finish() override;
    virtual void initialize(int stage) override;
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }

   private:
    cGate *getGate(isldirection::Direction direction, cGate::Type type, int index);
};

}  // namespace satellite
}  // namespace flora

#endif  // __FLORA_SATELLITE_SATELLITEROUTINGBASE_H_