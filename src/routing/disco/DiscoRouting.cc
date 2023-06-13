/*
 * DiscoRouting.cc
 *
 * Created on: Feb 04, 2023
 *     Author: Robin Ohs
 */

#include "DiscoRouting.h"

namespace flora {
namespace routing {

Define_Module(DiscoRouting);

void DiscoRouting::initialize(int stage) {
    RoutingBase::initialize(stage);
}

void DiscoRouting::handleTopologyChange() {
    Enter_Method("handleTopologyChange");
    auto dst = 265;
    auto src = 4;
    auto constellation = topologyControl->getSatellites();

    bool srcAsc = constellation.at(src)->isAscending();
    bool dstAsc = constellation.at(dst)->isAscending();

    if (srcAsc == true && dstAsc == true) {
        discoRoute(DiscoMode::A2A, topologyControl, src, dst);
    } else if (srcAsc == false && dstAsc == false) {
        discoRoute(DiscoMode::D2D, topologyControl, src, dst);
    } else if (srcAsc == true && dstAsc == false) {
        discoRoute(DiscoMode::A2D, topologyControl, src, dst);
    } else if (srcAsc == false && dstAsc == true) {
        discoRoute(DiscoMode::D2A, topologyControl, src, dst);
    }
}

ISLDirection DiscoRouting::routePacket(inet::Ptr<const RoutingHeader> rh, SatelliteRouting* callerSat) {
    Enter_Method("routePacket");
    return ISLDirection::RIGHT;
}

SendInformationRes DiscoRouting::getSendInformation(const core::MinHopsRes& res) {
    SendDirection dir = LEFT_UP;
    int hopsLeftUp = res.interPlaneHops.west + res.intraPlaneHops.upLeft;
    int hops = hopsLeftUp;
    // check west down
    int hopsLeftDown = res.interPlaneHops.west + res.intraPlaneHops.downLeft;
    if (hopsLeftDown < hops) {
        dir = LEFT_DOWN;
        hops = hopsLeftDown;
    }
    // check east up
    int hopsRightUp = res.interPlaneHops.east + res.intraPlaneHops.upRight;
    if (hopsRightUp < hops) {
        dir = RIGHT_UP;
        hops = hopsRightUp;
    }
    // check east down
    int hopsRightDown = res.interPlaneHops.east + res.intraPlaneHops.downRight;
    if (hopsRightDown < hops) {
        dir = RIGHT_DOWN;
        hops = hopsRightDown;
    }

    int hHorizontal;
    int hVertical;
    switch (dir) {
        case RIGHT_UP:
            hVertical = res.intraPlaneHops.upRight;
            hHorizontal = res.interPlaneHops.east;
            break;
        case RIGHT_DOWN:
            hVertical = res.intraPlaneHops.downRight;
            hHorizontal = res.interPlaneHops.east;
            break;
        case LEFT_UP:
            hVertical = res.intraPlaneHops.upLeft;
            hHorizontal = res.interPlaneHops.west;
            break;
        case LEFT_DOWN:
            hVertical = res.intraPlaneHops.downLeft;
            hHorizontal = res.interPlaneHops.west;
            break;
        default:
            error("Error in DiscoRouting::GetSendInformation(): Unhandled SendDirection.");
            break;
    }
    std::cout << "Min Hops:" << endl;
    std::cout << "  West-Up: " << hopsLeftUp << endl;
    std::cout << "  West-Down: " << hopsLeftDown << endl;
    std::cout << "  East-Up: " << hopsRightUp << endl;
    std::cout << "  East-Down: " << hopsRightDown << endl;
    std::cout << endl;
    return SendInformationRes{dir, hHorizontal, hVertical};
}

std::vector<int> DiscoRouting::discoRoute(DiscoMode mode, const topologycontrol::TopologyControlBase* tC, int src, int dst) {
    if (mode == DiscoMode::A2A) {
        return discoRouteA2A(tC, src, dst);
    } else if (mode == DiscoMode::D2D) {
        return discoRouteD2D(tC, src, dst);
    } else if (mode == DiscoMode::A2D) {
        return discoRouteA2D(tC, src, dst);
    } else if (mode == DiscoMode::D2A) {
        return discoRouteD2A(tC, src, dst);
    }
    throw new cRuntimeError("Error in DiscoRouting::discoRoute: Unexpected mode given.");
}

std::vector<int> DiscoRouting::discoRouteA2A(const topologycontrol::TopologyControlBase* tC, int src, int dst) {
    ASSERT(tC != nullptr);

    auto srcSat = tC->getSatellite(src);
    auto dstSat = tC->getSatellite(dst);
    auto raanDelta = tC->getRaanDelta();
    auto phaseDiff = tC->getPhaseDiff();
    auto phaseOffset = tC->getPhaseOffset();
    auto satsPerPlane = tC->getSatsPerPlane();
    auto numPlanes = tC->getPlaneCount();
    auto res = core::minHops(srcSat->getMnAnomaly(), srcSat->getRAAN(), dstSat->getMnAnomaly(), dstSat->getRAAN(), raanDelta, phaseDiff, phaseOffset);

    // Get send direction and min hop count
    auto sendInformationRes = getSendInformation(res);
    auto dir = sendInformationRes.dir;
    auto hHorizontal = sendInformationRes.hHorizontal;
    auto hVertical = sendInformationRes.hVertical;

    std::deque<int> route_s{src};
    std::deque<int> route_t{dst};

    // (o, i) -> i-th sat in plane o

    int i_p = srcSat->getPlane();
    int i_n = srcSat->getNumberInPlane();
    int j_p = dstSat->getPlane();
    int j_n = dstSat->getNumberInPlane();

    std::cout << "i_p" << i_p << ";"
              << "i_n" << i_n << ";" << endl;
    std::cout << "j_p" << j_p << ";"
              << "j_n" << j_n << ";" << endl;

    for (size_t time = 0; time < hHorizontal; time++) {
        int i_p_tmp;
        int j_p_tmp;
        switch (dir) {
            case LEFT_UP:
            case LEFT_DOWN: {
                i_p_tmp = core::fpmod(i_p - 1, numPlanes);
                j_p_tmp = core::fpmod(j_p + 1, numPlanes);
            } break;
            case RIGHT_UP:
            case RIGHT_DOWN: {
                i_p_tmp = core::fpmod(i_p + 1, numPlanes);
                j_p_tmp = core::fpmod(j_p - 1, numPlanes);
            } break;
        }
        int next_sat_s = tC->calculateSatelliteId(i_p_tmp, i_n);
        int next_sat_t = tC->calculateSatelliteId(j_p_tmp, j_n);
        std::cout << "S: Check S(" << i_p << "," << i_n << ";" << tC->calculateSatelliteId(i_p, i_n) << ") + S(" << i_p_tmp << "," << i_n << ";" << next_sat_s << ")" << endl;
        std::cout << "T: Check S(" << j_p << "," << j_n << ";" << tC->calculateSatelliteId(j_p, j_n) << ") + S(" << j_p_tmp << "," << j_n << ";" << next_sat_t << ")" << endl;
        double reward_s = abs(tC->findSatByPlaneAndNumberInPlane(i_p, i_n)->getLatitude() + tC->getSatellite(next_sat_s)->getLatitude());
        double reward_t = abs(tC->findSatByPlaneAndNumberInPlane(j_p, j_n)->getLatitude() + tC->getSatellite(next_sat_t)->getLatitude());
        std::cout << "Reward S: " << reward_s << endl;
        std::cout << "Reward T: " << reward_t << endl;
        if (reward_s < reward_t) {
            route_t.push_front(next_sat_t);
            j_p = j_p_tmp;
        } else {
            route_s.push_back(next_sat_s);
            i_p = i_p_tmp;
        }
    }

    VALIDATE(i_p == j_p);

    std::cout << "Start: " << tC->calculateSatelliteId(i_p, i_n) << " in plane" << endl;

    if (hVertical == 0) {
        route_t.pop_front();
    } else {
        int numInPlane = i_n;
        for (size_t i = 1; i < hVertical; i++) {
            switch (dir) {
                case RIGHT_UP:
                case LEFT_UP: {
                    numInPlane += 1;
                } break;
                case RIGHT_DOWN:
                case LEFT_DOWN: {
                    numInPlane -= 1;
                } break;
            }
            numInPlane = core::fpmod(numInPlane, satsPerPlane);
            std::cout << tC->calculateSatelliteId(i_p, numInPlane) << " in plane" << endl;
            route_s.push_back(tC->calculateSatelliteId(i_p, numInPlane));
        }
    }

    std::vector<int> v;

    v.insert(v.begin(), route_s.begin(), route_s.end());
    v.insert(v.end(), route_t.begin(), route_t.end());

    std::cout << "Route: " << flora::core::utils::vector::toString(v.begin(), v.end()) << endl;

    return v;
}

std::vector<int> DiscoRouting::discoRouteD2D(const topologycontrol::TopologyControlBase* tC, int src, int dst) {
    ASSERT(tC != nullptr);

    auto srcSat = tC->getSatellite(src);
    auto dstSat = tC->getSatellite(dst);
    auto raanDelta = tC->getRaanDelta();
    auto phaseDiff = tC->getPhaseDiff();
    auto phaseOffset = tC->getPhaseOffset();
    auto satsPerPlane = tC->getSatsPerPlane();
    auto numPlanes = tC->getPlaneCount();
    auto res = core::minHops(srcSat->getMnAnomaly(), srcSat->getRAAN(), dstSat->getMnAnomaly(), dstSat->getRAAN(), raanDelta, phaseDiff, phaseOffset);

    // Get send direction and min hop count
    auto sendInformationRes = getSendInformation(res);
    auto dir = sendInformationRes.dir;
    auto hHorizontal = sendInformationRes.hHorizontal;
    auto hVertical = sendInformationRes.hVertical;

    std::deque<int> route_s{src};
    std::deque<int> route_t{dst};

    // (o, i) -> i-th sat in plane o

    int i_p = srcSat->getPlane();
    int i_n = srcSat->getNumberInPlane();
    int j_p = dstSat->getPlane();
    int j_n = dstSat->getNumberInPlane();

    std::cout << "i_p" << i_p << ";"
              << "i_n" << i_n << ";" << endl;
    std::cout << "j_p" << j_p << ";"
              << "j_n" << j_n << ";" << endl;

    for (size_t time = 0; time < hHorizontal; time++) {
        int i_p_tmp;
        int j_p_tmp;
        switch (dir) {
            case LEFT_UP:
            case LEFT_DOWN: {
                i_p_tmp = core::fpmod(i_p - 1, numPlanes);
                j_p_tmp = core::fpmod(j_p + 1, numPlanes);
            } break;
            case RIGHT_UP:
            case RIGHT_DOWN: {
                i_p_tmp = core::fpmod(i_p + 1, numPlanes);
                j_p_tmp = core::fpmod(j_p - 1, numPlanes);
            } break;
        }
        int next_sat_s = tC->calculateSatelliteId(i_p_tmp, i_n);
        int next_sat_t = tC->calculateSatelliteId(j_p_tmp, j_n);
        std::cout << "S: Check S(" << i_p << "," << i_n << ";" << tC->calculateSatelliteId(i_p, i_n) << ") + S(" << i_p_tmp << "," << i_n << ";" << next_sat_s << ")" << endl;
        std::cout << "T: Check S(" << j_p << "," << j_n << ";" << tC->calculateSatelliteId(j_p, j_n) << ") + S(" << j_p_tmp << "," << j_n << ";" << next_sat_t << ")" << endl;
        double reward_s = abs(tC->findSatByPlaneAndNumberInPlane(i_p, i_n)->getLatitude() + tC->getSatellite(next_sat_s)->getLatitude());
        double reward_t = abs(tC->findSatByPlaneAndNumberInPlane(j_p, j_n)->getLatitude() + tC->getSatellite(next_sat_t)->getLatitude());
        std::cout << "Reward S: " << reward_s << endl;
        std::cout << "Reward T: " << reward_t << endl;
        if (reward_s < reward_t) {
            route_t.push_front(next_sat_t);
            j_p = j_p_tmp;
        } else {
            route_s.push_back(next_sat_s);
            i_p = i_p_tmp;
        }
    }

    VALIDATE(i_p == j_p);

    std::cout << "Start: " << tC->calculateSatelliteId(i_p, i_n) << " in plane" << endl;

    if (hVertical == 0) {
        route_t.pop_front();
    } else {
        int numInPlane = i_n;
        for (size_t i = 1; i < hVertical; i++) {
            switch (dir) {
                case RIGHT_UP:
                case LEFT_UP: {
                    numInPlane += 1;
                } break;
                case RIGHT_DOWN:
                case LEFT_DOWN: {
                    numInPlane -= 1;
                } break;
            }
            numInPlane = core::fpmod(numInPlane, satsPerPlane);
            std::cout << tC->calculateSatelliteId(i_p, numInPlane) << " in plane" << endl;
            route_s.push_back(tC->calculateSatelliteId(i_p, numInPlane));
        }
    }

    std::vector<int> v;

    v.insert(v.begin(), route_s.begin(), route_s.end());
    v.insert(v.end(), route_t.begin(), route_t.end());

    std::cout << "Route: " << flora::core::utils::vector::toString(v.begin(), v.end()) << endl;

    return v;
}

std::vector<int> DiscoRouting::discoRouteA2D(const topologycontrol::TopologyControlBase* tC, int src, int dst) {
    ASSERT(tC != nullptr);

    auto srcSat = tC->getSatellite(src);
    auto dstSat = tC->getSatellite(dst);
    auto raanDelta = tC->getRaanDelta();
    auto phaseDiff = tC->getPhaseDiff();
    auto phaseOffset = tC->getPhaseOffset();
    auto satsPerPlane = tC->getSatsPerPlane();
    auto numPlanes = tC->getPlaneCount();
    auto res = core::minHops(srcSat->getMnAnomaly(), srcSat->getRAAN(), dstSat->getMnAnomaly(), dstSat->getRAAN(), raanDelta, phaseDiff, phaseOffset);

    // Get send direction and min hop count
    auto sendInformationRes = getSendInformation(res);
    auto dir = sendInformationRes.dir;
    auto hHorizontal = sendInformationRes.hHorizontal;
    auto hVertical = sendInformationRes.hVertical;

    std::deque<int> route_s{src};
    std::deque<int> route_t{dst};

    int i_p = srcSat->getPlane();
    int i_n = srcSat->getNumberInPlane();
    int j_p = dstSat->getPlane();
    int j_n = dstSat->getNumberInPlane();

    std::cout << "i_p" << i_p << ";"
              << "i_n" << i_n << ";" << endl;
    std::cout << "j_p" << j_p << ";"
              << "j_n" << j_n << ";" << endl;

    for (size_t time = 0; time < hVertical; time++) {
        int i_n_tmp;
        int j_n_tmp;
        switch (dir) {
            case LEFT_UP:
            case LEFT_DOWN: {
                i_n_tmp = core::fpmod(i_n - 1, satsPerPlane);
                j_n_tmp = core::fpmod(j_n + 1, satsPerPlane);
            } break;
            case RIGHT_UP:
            case RIGHT_DOWN: {
                i_n_tmp = core::fpmod(i_n + 1, satsPerPlane);
                j_n_tmp = core::fpmod(j_n - 1, satsPerPlane);
            } break;
        }
        int next_sat_s = tC->calculateSatelliteId(i_p, i_n_tmp);
        int next_sat_t = tC->calculateSatelliteId(j_p, j_n_tmp);
        double reward_s = abs(tC->findSatByPlaneAndNumberInPlane(i_p, i_n)->getLatitude() + tC->getSatellite(next_sat_s)->getLatitude());
        double reward_t = abs(tC->findSatByPlaneAndNumberInPlane(j_p, j_n)->getLatitude() + tC->getSatellite(next_sat_t)->getLatitude());
        std::cout << "S: Check S(" << i_p << "," << i_n << ";" << tC->calculateSatelliteId(i_p, i_n) << ") -> S(" << i_p << "," << i_n_tmp << ";" << next_sat_s << "): Reward = "
                  << reward_s << endl;
        std::cout << "T: Check S(" << j_p << "," << j_n << ";" << tC->calculateSatelliteId(j_p, j_n) << ") -> S(" << j_p << "," << j_n_tmp << ";" << next_sat_t << "): Reward = "
                  << reward_t << endl;
        if (reward_s < reward_t) {
            std::cout << "  -> Select s: S(" << i_p << "," << i_n_tmp << ";" << next_sat_s << ")" << endl;
            route_s.push_back(next_sat_s);
            i_n = i_n_tmp;
        } else {
            std::cout << "  -> Select t: S(" << j_p << "," << j_n_tmp << ";" << next_sat_t << ")" << endl;
            route_t.push_front(next_sat_t);
            j_n = j_n_tmp;
        }
    }

    std::cout << "i_p" << i_p << ";"
              << "i_n" << i_n << ";" << endl;
    std::cout << "j_p" << j_p << ";"
              << "j_n" << j_n << ";" << endl;

    // asserts needs to care if its the 0-th plane -> then we need to care about the phaseOffset
    if (i_p != 0 && j_p != 0) {
        VALIDATE(i_n == j_n);
    } else {
        VALIDATE(i_n == core::fpmod(j_n - tC->getInterPlaneSpacing(), satsPerPlane) || j_n == core::fpmod(i_n - tC->getInterPlaneSpacing(), satsPerPlane));
    }

    std::cout << "Start: S(" << i_p << "," << i_n << ";" << tC->calculateSatelliteId(i_p, i_n) << ") in plane" << endl;

    if (hHorizontal == 0) {
        route_t.pop_front();
    } else {
        int plane = i_p;
        for (size_t i = 1; i < hHorizontal; i++) {
            switch (dir) {
                case RIGHT_UP:
                case LEFT_UP: {
                    plane += 1;
                } break;
                case RIGHT_DOWN:
                case LEFT_DOWN: {
                    plane -= 1;
                } break;
            }
            plane = core::fpmod(plane, numPlanes);
            std::cout << "-> S(" << plane << "," << i_n << ";" << tC->calculateSatelliteId(plane, i_n) << ")" << endl;
            route_s.push_back(tC->calculateSatelliteId(plane, i_n));
        }
    }

    std::vector<int> v;

    v.insert(v.begin(), route_s.begin(), route_s.end());
    v.insert(v.end(), route_t.begin(), route_t.end());

    std::cout << "Route: " << flora::core::utils::vector::toString(v.begin(), v.end()) << endl;

    return v;
}

std::vector<int> DiscoRouting::discoRouteD2A(const topologycontrol::TopologyControlBase* tC, int src, int dst) {
    ASSERT(tC != nullptr);

    auto srcSat = tC->getSatellite(src);
    auto dstSat = tC->getSatellite(dst);
    auto raanDelta = tC->getRaanDelta();
    auto phaseDiff = tC->getPhaseDiff();
    auto phaseOffset = tC->getPhaseOffset();
    auto satsPerPlane = tC->getSatsPerPlane();
    auto numPlanes = tC->getPlaneCount();
    auto res = core::minHops(srcSat->getMnAnomaly(), srcSat->getRAAN(), dstSat->getMnAnomaly(), dstSat->getRAAN(), raanDelta, phaseDiff, phaseOffset);

    // Get send direction and min hop count
    auto sendInformationRes = getSendInformation(res);
    auto dir = sendInformationRes.dir;
    auto hHorizontal = sendInformationRes.hHorizontal;
    auto hVertical = sendInformationRes.hVertical;

    std::deque<int> route_s{src};
    std::deque<int> route_t{dst};

    int i_p = srcSat->getPlane();
    int i_n = srcSat->getNumberInPlane();
    int j_p = dstSat->getPlane();
    int j_n = dstSat->getNumberInPlane();

    std::cout << "i_p" << i_p << ";"
              << "i_n" << i_n << ";" << endl;
    std::cout << "j_p" << j_p << ";"
              << "j_n" << j_n << ";" << endl;

    for (size_t time = 0; time < hVertical; time++) {
        int i_n_tmp;
        int j_n_tmp;
        switch (dir) {
            case LEFT_UP:
            case LEFT_DOWN: {
                i_n_tmp = core::fpmod(i_n - 1, satsPerPlane);
                j_n_tmp = core::fpmod(j_n + 1, satsPerPlane);
            } break;
            case RIGHT_UP:
            case RIGHT_DOWN: {
                i_n_tmp = core::fpmod(i_n + 1, satsPerPlane);
                j_n_tmp = core::fpmod(j_n - 1, satsPerPlane);
            } break;
        }
        int next_sat_s = tC->calculateSatelliteId(i_p, i_n_tmp);
        int next_sat_t = tC->calculateSatelliteId(j_p, j_n_tmp);
        double reward_s = abs(tC->findSatByPlaneAndNumberInPlane(i_p, i_n)->getLatitude() + tC->getSatellite(next_sat_s)->getLatitude());
        double reward_t = abs(tC->findSatByPlaneAndNumberInPlane(j_p, j_n)->getLatitude() + tC->getSatellite(next_sat_t)->getLatitude());

        std::cout << "S: Check S(" << i_p << "," << i_n << ";" << tC->calculateSatelliteId(i_p, i_n) << ") -> S(" << i_p << "," << i_n_tmp << ";" << next_sat_s << "): Reward = "
                  << reward_s << endl;
        std::cout << "T: Check S(" << j_p << "," << j_n << ";" << tC->calculateSatelliteId(j_p, j_n) << ") -> S(" << j_p << "," << j_n_tmp << ";" << next_sat_t << "): Reward = "
                  << reward_t << endl;

        if (reward_s < reward_t) {
            std::cout << "  -> Select s: S(" << i_p << "," << i_n_tmp << ";" << next_sat_s << ")" << endl;
            route_s.push_back(next_sat_s);
            i_n = i_n_tmp;
        } else {
            std::cout << "  -> Select t: S(" << j_p << "," << j_n_tmp << ";" << next_sat_t << ")" << endl;
            route_t.push_front(next_sat_t);
            j_n = j_n_tmp;
        }
    }

    std::cout << "i_p" << i_p << ";"
              << "i_n" << i_n << ";" << endl;
    std::cout << "j_p" << j_p << ";"
              << "j_n" << j_n << ";" << endl;

    // asserts needs to care if its the 0-th plane -> then we need to care about the phaseOffset
    // if (i_p != 0 && j_p != 0) {
    //     VALIDATE(i_n == j_n);
    // } else {
    //     VALIDATE(i_n == core::fpmod(j_n + tC->getInterPlaneSpacing(), satsPerPlane) || j_n == core::fpmod(i_n + tC->getInterPlaneSpacing(), satsPerPlane));
    // }

    std::cout << "Start: S(" << i_p << "," << i_n << ";" << tC->calculateSatelliteId(i_p, i_n) << ") in plane" << endl;

    if (hHorizontal == 0) {
        route_t.pop_front();
    } else {
        int plane = i_p;
        for (size_t i = 1; i < hHorizontal; i++) {
            switch (dir) {
                case RIGHT_UP:
                case LEFT_UP: {
                    plane += 1;
                } break;
                case RIGHT_DOWN:
                case LEFT_DOWN: {
                    plane -= 1;
                } break;
            }
            plane = core::fpmod(plane, numPlanes);
            std::cout << "-> S(" << plane << "," << i_n << ";" << tC->calculateSatelliteId(plane, i_n) << ")" << endl;
            route_s.push_back(tC->calculateSatelliteId(plane, i_n));
        }
    }

    std::vector<int> v;

    v.insert(v.begin(), route_s.begin(), route_s.end());
    v.insert(v.end(), route_t.begin(), route_t.end());

    std::cout << "Route: " << flora::core::utils::vector::toString(v.begin(), v.end()) << endl;

    return v;
}

}  // namespace routing
}  // namespace flora
