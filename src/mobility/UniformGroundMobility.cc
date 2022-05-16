/*
 * UniformGroundMobility.cc
 *
 *  Created on: Mar 24, 2022
 *      Author: diego
 */


#include "UniformGroundMobility.h"
#include "libnorad/cEcef.h"
#include "libnorad/globals.h"
#include <cmath>

namespace flora {

Define_Module(UniformGroundMobility);

using namespace inet;

UniformGroundMobility::UniformGroundMobility()
{
    longitude = 0.0;
    latitude = 0.0;
    centerLatitude = 0.0;
    centerLongitude = 0.0;
    radius = 0.0;
    mapx = 0;
    mapy = 0;
}

void UniformGroundMobility::initialize(int stage)
{
    StationaryMobility::initialize(stage);
    EV << "initializing UniformGroundMobility stage " << stage << endl;
    if (stage == 0)
    {
        mapx = std::atoi(getParentModule()->getParentModule()->getDisplayString().getTagArg("bgb", 0));
        mapy = std::atoi(getParentModule()->getParentModule()->getDisplayString().getTagArg("bgb", 1));
        centerLatitude = par("centerLatitude");
        centerLongitude = par("centerLongitude");
        radius = par("radius");

        double r = radius * sqrt(cComponent::uniform(0, 1));
        double a = cComponent::uniform(0, TWOPI);

        longitude = centerLongitude + r * cos(a);
        latitude = centerLatitude + r * sin(a);
    }
}

double UniformGroundMobility::getDistance(const double& refLatitude, const double& refLongitude, const double& refAltitude) const
{
    //could change altitude to real value
    cEcef ecefSourceCoord = cEcef(latitude, longitude, 0);
    cEcef ecefDestCoord = cEcef(refLatitude, refLongitude, refAltitude);
    return ecefSourceCoord.getDistance(ecefDestCoord);
}

double UniformGroundMobility::getLongitude() const
{
    return longitude;
}

double UniformGroundMobility::getLatitude() const
{
    return latitude;
}

Coord& UniformGroundMobility::getCurrentPosition()
{
    return lastPosition;
}

void UniformGroundMobility::setInitialPosition()
{
    lastPosition.x = ((mapx * longitude) / 360) + (mapx / 2);
    lastPosition.x = static_cast<int>(lastPosition.x) % static_cast<int>(mapx);
    lastPosition.y = ((-mapy * latitude) / 180) + (mapy / 2);
    lastPosition = Coord(lastPosition.x, lastPosition.y, 0);
}

} // namespace inet
