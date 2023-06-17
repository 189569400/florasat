/*
 * MinHopCount.h
 *
 * Created on: Jun 09, 2023
 *     Author: Robin Ohs
 */

#ifndef __FLORA_ROUTING_CORE_MINHOPCOUNT_H_
#define __FLORA_ROUTING_CORE_MINHOPCOUNT_H_

#include "core/utils/VectorUtils.h"
#include "satellite/SatelliteRoutingBase.h"
#include "topologycontrol/TopologyControlBase.h"

using namespace flora::satellite;

namespace flora {
namespace routing {
namespace core {

enum SendDirection {
    LEFT_UP = 0,
    LEFT_DOWN = 1,
    RIGHT_UP = 2,
    RIGHT_DOWN = 3,
};

/** @brief fmod which returns remainder in set [0,divisor[ */
double fpmod(double dividend, double divisor = 360.0);

/** @brief Rounds half away from zero. E.g., roundC(3.5) = 4, roundC(-2.5) = 3; */
double roundCom(double x);

struct MinInterPlaneHopsRes {
    int west;
    int east;
};
/** @brief Computes the minimum number of inter-plane hops Hh between two raan. */
MinInterPlaneHopsRes minInterplaneHops(double raanSrc, double raanDst, double raanDiff);

struct MinIntraPlaneHopsRes {
    int upLeft;
    int upRight;
    int downLeft;
    int downRight;
};
/** @brief Computes the minimum number of inter-plane hops Hh between two satellites. */
MinIntraPlaneHopsRes minIntraPlaneHops(MinInterPlaneHopsRes interPlaneHops, double uSrc, double uDst, double phaseDiff, double f);

struct MinHopsRes {
    MinInterPlaneHopsRes interPlaneHops;
    MinIntraPlaneHopsRes intraPlaneHops;
    int hopsWestUp;
    int hopsWestDown;
    int hopsEastUp;
    int hopsEastDown;
    int minHops;
    core::SendDirection dir;
    int hHorizontal;
    int hVertical;
};
/** @brief Calculates the minimal hops for all directions. */
MinHopsRes minHops(double uSrc, double raanSrc, double uDst, double raanDst, double raanDiff, double phaseDiff, double f);

// (o, i) => ith satellite in orbital plane o
// Q satellites per plane, inclination 𝛼, altitude h, F relative spacing between sats in adjacent planes
// F usually given as phasing factor F € {0, ..., P-1}
// Walker delta: PQ/P/F
// RAAN difference:     ΔΩ = 2𝜋/𝑃 € [0,2𝜋]      -> how far neighbouring planes are from each other
// Phase difference:    ΔΦ = 2𝜋/Q € [0,2𝜋]      -> difference in arg of latitude
// Phase offset:        Δf = 2𝜋F/PQ € [0,2𝜋[    -> difference in arg of latitude between horizontal neighbours

}  // namespace core
}  // namespace routing
}  // namespace flora

#endif  // __FLORA_ROUTING_CORE_MINHOPCOUNT_H_