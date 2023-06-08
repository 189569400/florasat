/*
 * MinimumHopCount.cc
 *
 * Created on: Jun 07, 2023
 *     Author: Robin Ohs
 */

#include "MinimumHopCount.h"

namespace flora {
namespace routing {
namespace core {
namespace mhca {

double fpmod(double dividend) {
    auto res = fmod(dividend, 360.0);
    return res < 0 ? 360.0 + res : res;
}

double roundCom(double x) {
    bool neg = x < 0;
    double val = floor(abs(x) + 0.5);
    return neg ? -1 * val : val;
}

MinInterPlaneHopsRes minInterplaneHops(double raanSrc, double raanDst, double raanDiff) {
    double l0 = fpmod(raanDst - raanSrc);
    int west = roundCom((360.0 - l0) / raanDiff);
    int east = roundCom(l0 / raanDiff);
    return MinInterPlaneHopsRes{west, east};
}

MinIntraPlaneHopsRes minIntraPlaneHops(MinInterPlaneHopsRes interPlaneHops, double uSrc, double uDst, double phaseDiff, double f) {
    double uRight = fpmod(uDst - uSrc - interPlaneHops.east * f);
    double uLeft = fpmod(uDst - uSrc + interPlaneHops.west * f);

    int upLeft = roundCom(uLeft / phaseDiff);
    int upRight = roundCom(uRight / phaseDiff);
    int downLeft = roundCom((360 - uLeft) / phaseDiff);
    int downRight = roundCom((360 - uRight) / phaseDiff);

    return MinIntraPlaneHopsRes{upLeft, upRight, downLeft, downRight};
}

MinHopsRes minHops(double uSrc, double raanSrc, double uDst, double raanDst, double raanDiff, double phaseDiff, double f) {
    auto interPlaneRes = minInterplaneHops(raanSrc, raanDst, raanDiff);
    auto intraPlaneRes = minIntraPlaneHops(interPlaneRes, uSrc, uDst, phaseDiff, f);
    auto leftUp = interPlaneRes.west + intraPlaneRes.upLeft;
    auto leftDown = interPlaneRes.west + intraPlaneRes.downLeft;
    auto rightUp = interPlaneRes.east + intraPlaneRes.upRight;
    auto rightDown = interPlaneRes.east + intraPlaneRes.downRight;
    return MinHopsRes{leftUp, leftDown, rightUp, rightDown};
}

}  // namespace mhca
}  // namespace core
}  // namespace routing
}  // namespace flora