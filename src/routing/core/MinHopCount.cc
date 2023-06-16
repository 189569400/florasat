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
    double l0_w = fpmod(360.0 - l0_e);
    int west = roundCom(l0_w / raanDiff);
    int east = roundCom(l0_e / raanDiff);
    // std::cout << "InterPlaneHops:" << endl;
    // std::cout << "  RAAN-Src: " << raanSrc << endl;
    // std::cout << "  RAAN-Dst: " << raanDst << endl;
    // std::cout << "  raanDiff: " << raanDiff << endl;
    // std::cout << "  l0_e: " << l0_e << endl;
    // std::cout << "  l0_w: " << l0_w << endl;
    // std::cout << "  hops east: " << east << endl;
    // std::cout << "  hops west: " << west << endl;
    // std::cout << endl;
    return MinInterPlaneHopsRes{west, east};
}

MinIntraPlaneHopsRes minIntraPlaneHops(MinInterPlaneHopsRes interPlaneHops, double uSrc, double uDst, double phaseDiff, double f) {
    double phaseAngleDiff = uDst - uSrc;
    double phasingAngleByEastInterPlaneHops = interPlaneHops.east * f;
    double phasingAngleByWestInterPlaneHops = interPlaneHops.west * f;

    double delta_U_East = fpmod(phaseAngleDiff - phasingAngleByEastInterPlaneHops);
    double delta_U_West = fpmod(phaseAngleDiff + phasingAngleByWestInterPlaneHops);

    int hV_UpWest = roundCom(delta_U_West / phaseDiff);
    int hV_UpEast = roundCom(delta_U_East / phaseDiff);

    int hV_DownWest = roundCom((360.0 - delta_U_West) / phaseDiff);
    int hV_DownEast = roundCom((360.0 - delta_U_East) / phaseDiff);

    // std::cout << "IntraPlaneHops:" << endl;
    // std::cout << "  PhasingAngle-Src (uSrc): " << uSrc << endl;
    // std::cout << "  PhasingAngle-Dst (uDst): " << uDst << endl;
    // std::cout << "  PhasingAngle-Diff: " << phaseAngleDiff << endl;
    // std::cout << "  PhasingAngle-FPmodDiff: " << fpmod(phaseAngleDiff) << endl;
    // std::cout << "    -> Jumps Up: " << roundCom(fpmod(phaseAngleDiff - phasingAngleByEastInterPlaneHops) / phaseDiff) << endl;
    // std::cout << "    -> Jumps Down: " << roundCom((360.0 - fpmod(phaseAngleDiff + phasingAngleByWestInterPlaneHops)) / phaseDiff) << endl;
    // std::cout << "  PhasingAngle-East-Jumps: " << phasingAngleByEastInterPlaneHops << endl;
    // std::cout << "  PhasingAngle-West-Jumps: " << phasingAngleByWestInterPlaneHops << endl;
    // std::cout << endl;
    // std::cout << "  delta_U_East: " << delta_U_East << "(" << phaseAngleDiff << "-" << phasingAngleByEastInterPlaneHops << ")" << endl;
    // std::cout << "  delta_U_West: " << delta_U_West << "(" << phaseAngleDiff << "+" << phasingAngleByWestInterPlaneHops << ")" << endl;
    // std::cout << endl;
    // std::cout << "  hV_UpLeft: " << hV_UpWest << endl;
    // std::cout << "  hV_UpRight: " << hV_UpEast << endl;
    // std::cout << "  hV_DownLeft: " << hV_DownWest << endl;
    // std::cout << "  hV_DownRight: " << hV_DownEast << endl;
    // std::cout << endl;
    return MinIntraPlaneHopsRes{hV_UpWest, hV_UpEast, hV_DownWest, hV_DownEast};
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