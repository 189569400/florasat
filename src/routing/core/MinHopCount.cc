/*
 * DiscoRoute.cc
 *
 * Created on: Jun 09, 2023
 *     Author: Robin Ohs
 */

#include "MinHopCount.h"

namespace flora {
namespace routing {
namespace core {

double fpmod(double dividend, double divisor) {
    auto res = fmod(dividend, divisor);
    return res < 0 ? divisor + res : res;
}

double roundCom(double x) {
    bool neg = x < 0;
    double val = floor(abs(x) + 0.5);
    return neg ? -1 * val : val;
}

MinInterPlaneHopsRes minInterplaneHops(double raanSrc, double raanDst, double raanDiff) {
    double l0_e = fpmod(raanDst - raanSrc);
    double l0_w = 360.0 - l0_e;
    int west = roundCom(l0_w / raanDiff);
    int east = roundCom(l0_e / raanDiff);
    std::cout << "InterPlaneHops:" << endl;
    std::cout << "  RAAN-Src: " << raanSrc << endl;
    std::cout << "  RAAN-Dst: " << raanDst << endl;
    std::cout << "  raanDiff: " << raanDiff << endl;
    std::cout << "  l0_e: " << l0_e << endl;
    std::cout << "  l0_w: " << l0_w << endl;
    std::cout << "  hops east: " << east << endl;
    std::cout << "  hops west: " << west << endl;
    std::cout << endl;
    return MinInterPlaneHopsRes{west, east};
}

MinIntraPlaneHopsRes minIntraPlaneHops(MinInterPlaneHopsRes interPlaneHops, double uSrc, double uDst, double phaseDiff, double f) {
    double phaseAngleDiff = uDst - uSrc;
    double delta_U_Right = fpmod(phaseAngleDiff - interPlaneHops.east * f);
    double delta_U_Left = fpmod(phaseAngleDiff + interPlaneHops.west * f);

    int hV_UpLeft = roundCom(delta_U_Left / phaseDiff);
    int hV_UpRight = roundCom(delta_U_Right / phaseDiff);

    int hV_DownLeft = roundCom((360.0 - hV_UpLeft) / phaseDiff);
    int hV_DownRight = roundCom((360.0 - hV_UpRight) / phaseDiff);

    std::cout << "IntraPlaneHops:" << endl;
    std::cout << "  PhasingAngle-Src (uSrc): " << uSrc << endl;
    std::cout << "  PhasingAngle-Dst (uDst): " << uDst << endl;
    std::cout << "  PhasingAngle-Diff: " << phaseAngleDiff << endl;
    std::cout << endl;
    std::cout << "  delta_U_Right: " << delta_U_Right << endl;
    std::cout << "  delta_U_Left: " << delta_U_Left << endl;
    std::cout << endl;
    std::cout << "  hV_UpLeft: " << hV_UpLeft << endl;
    std::cout << "  hV_UpRight: " << hV_UpRight << endl;
    std::cout << "  hV_DownLeft: " << hV_DownLeft << endl;
    std::cout << "  hV_DownRight: " << hV_DownRight << endl;
    std::cout << endl;
    return MinIntraPlaneHopsRes{hV_UpLeft, hV_UpRight, hV_DownLeft, hV_DownRight};
}

MinHopsRes minHops(double uSrc, double raanSrc, double uDst, double raanDst, double raanDiff, double phaseDiff, double f) {
    auto interPlaneRes = minInterplaneHops(raanSrc, raanDst, raanDiff);
    auto intraPlaneRes = minIntraPlaneHops(interPlaneRes, uSrc, uDst, phaseDiff, f);
    // auto leftUp = interPlaneRes.west + intraPlaneRes.upLeft;
    // auto leftDown = interPlaneRes.west + intraPlaneRes.downLeft;
    // auto rightUp = interPlaneRes.east + intraPlaneRes.upRight;
    // auto rightDown = interPlaneRes.east + intraPlaneRes.downRight;
    return MinHopsRes{interPlaneRes, intraPlaneRes};
}

}  // namespace core
}  // namespace routing
}  // namespace flora