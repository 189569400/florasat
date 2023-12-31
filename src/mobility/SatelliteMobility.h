//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

#ifndef MOBILITY_SATELLITEMOBILITY_H_
#define MOBILITY_SATELLITEMOBILITY_H_

#include <inet/mobility/base/LineSegmentsMobilityBase.h>

#include <cmath>
#include <ctime>

#include "../libnorad/cEcef.h"
#include "../libnorad/cJulian.h"
#include "INorad.h"
#include "NoradA.h"
#include "inet/common/INETDefs.h"

//-----------------------------------------------------
// Class: SatelliteMobility
//
// Realizes the SatelliteMobility mobility module - provides methods to get and set
// the position of a satellite module and resets the satellite position when
// it gets outside the playground. Code taken from OS3 so that the model can
// work without altering the OS3 code itself.
//-----------------------------------------------------

using namespace inet;

namespace flora {

class SatelliteMobility : public LineSegmentsMobilityBase {
   public:
    SatelliteMobility();

    // returns x-position of satellite on playground (not longitude!)
    virtual double getPositionX() const { return lastPosition.x; };

    // returns y-position of satellite on playground (not latitude!)
    virtual double getPositionY() const { return lastPosition.y; };

    // returns the altitude of the satellite.
    virtual double getAltitude() const;

    virtual bool isOnSameOrbitalPlane(double raan, double inclination);
    // returns the elevation for the satellite in degrees
    virtual double getElevation(const double& refLatitude, const double& refLongitude, const double& refAltitude = -9999) const;

    // returns the azimuth from satellite to reference point in degrees
    virtual double getAzimuth(const double& refLatitude, const double& refLongitude, const double& refAltitude = -9999) const;

    // returns the Euclidean distance from satellite to reference point
    virtual double getDistance(const double& refLatitude, const double& refLongitude, const double& refAltitude = -9999) const;

    // returns satellite latitude
    virtual double getLatitude() const;

    // returns satellite longitude
    virtual double getLongitude() const;

    // returns horizontal satellite position (x) in canvas
    double getXCanvas(double lon) const;

    // returns vertical satellite position (y) in canvas
    double getYCanvas(double lat) const;

   protected:
    INorad* noradModule;
    int mapX, mapY;
    double transmitPower;
    bool displaySpanArea;
    cPolygonFigure* polygon;
    const int nPoints = 51;
    cCanvas* networkCanvas = nullptr;
    cMessage* refreshArea = nullptr;

    int effectiveSlant = 0;

    virtual int numInitStages() const override { return NUM_INIT_STAGES; }

    // initialize module
    // - creates a reference to the Norad moudule
    // - timestamps and initial position on playground are managed here.
    virtual void initialize(int stage) override;

    virtual void initializePosition() override;

    // sets the position of satellite
    // - sets the target position for the satellite
    // - the position is fetched from the Norad module with reference to the current timestamp
    virtual void setTargetPosition() override;

    // resets the position of the satellite
    // - wraps around the position of the satellite if it reaches the end of the playground
    virtual void fixIfHostGetsOutside();

    // implements basic satellite movement on map
    virtual void move() override;

    // move satellite and update span area
    virtual void handleSelfMessage(cMessage* msg) override;

    // remove all points from span area polygon
    void removeAllPoints();

    // set all points from span area polygon
    void setAllPoints();
};

}  // namespace flora

#endif
