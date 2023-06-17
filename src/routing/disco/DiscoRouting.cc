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

    double raanDelta = topologyControl->getRaanDelta();
    double phaseDiff = topologyControl->getPhaseDiff();
    double phaseOffset = topologyControl->getPhaseOffset();
    // auto srcSat = topologyControl->getSatellite(2);
    // auto dstSat = topologyControl->getSatellite(266);
    // auto res = core::minHops(srcSat->getMnAnomaly(), srcSat->getRAAN(), dstSat->getMnAnomaly(), dstSat->getRAAN(), raanDelta, phaseDiff, phaseOffset);
    // discoRouteA2D(res, topologyControl, srcSat->getId(), dstSat->getId());

    for (size_t src = 0; src < topologyControl->getNumberOfSatellites(); src++) {
        // get src sat related data
        auto srcSat = topologyControl->getSatellite(src);
        auto srcAsc = srcSat->isAscending();
        auto forwardingTable = static_cast<ForwardingTable*>(srcSat->getSubmodule("forwardingTable"));
        forwardingTable->clearRoutes();

        // iterate over all groundstations
        for (size_t gsId = 0; gsId < topologyControl->getNumberOfGroundstations(); gsId++) {
            auto gs = topologyControl->getGroundstationInfo(gsId);

            SatelliteRoutingBase* nearestSat = nullptr;
            core::MinHopsRes minHops;
            int smallestHopCount = 999999999;

            for (auto pLastSat : gs->getSatellites()) {
                auto dstSat = topologyControl->getSatellite(pLastSat);
                auto res = core::minHops(srcSat->getMnAnomaly(), srcSat->getRAAN(), dstSat->getMnAnomaly(), dstSat->getRAAN(), raanDelta, phaseDiff, phaseOffset);
                if (res.minHops < smallestHopCount) {
                    nearestSat = dstSat;
                    minHops = res;
                    smallestHopCount = res.minHops;
                }
            }

            VALIDATE(nearestSat != nullptr);

            // update forwarding table accordingly
            if (nearestSat->getId() == src) {
                forwardingTable->setRoute(gsId, isldirection::GROUNDLINK);
            } else {
                int nextSatId;

                if (srcAsc == nearestSat->isAscending()) {
                    nextSatId = discoRouteA2A(minHops, topologyControl, src, nearestSat->getId())[1];
                } else {
                    nextSatId = discoRouteA2D(minHops, topologyControl, src, nearestSat->getId())[1];
                }

                if (srcSat->hasLeftSat() && srcSat->getLeftSatId() == nextSatId) {
                    forwardingTable->setRoute(gsId, isldirection::LEFT);
                } else if (srcSat->hasUpSat() && srcSat->getUpSatId() == nextSatId) {
                    forwardingTable->setRoute(gsId, isldirection::UP);
                } else if (srcSat->hasRightSat() && srcSat->getRightSatId() == nextSatId) {
                    forwardingTable->setRoute(gsId, isldirection::RIGHT);
                } else if (srcSat->hasDownSat() && srcSat->getDownSatId() == nextSatId) {
                    forwardingTable->setRoute(gsId, isldirection::DOWN);
                } else {
                    error("Path not found.");
                }
            }
        }
    }
}

ISLDirection DiscoRouting::routePacket(inet::Ptr<const RoutingHeader> rh, SatelliteRouting* callerSat) {
    Enter_Method("routePacket");

    // check that its no unsupported packet
    flora::Type type = rh->getType();
    if(type != flora::Type::G2G) {
        error("Unsupported Packet received!");
    }
    auto forwardingTable = static_cast<ForwardingTable*>(callerSat->getSubmodule("forwardingTable"));
    return forwardingTable->getNextHop(rh->getDstGs());
}

std::vector<int> DiscoRouting::discoRouteA2A(const core::MinHopsRes& minHops, const topologycontrol::TopologyControlBase* tC, int src, int dst) {
    ASSERT(tC != nullptr);

    auto srcSat = tC->getSatellite(src);
    auto dstSat = tC->getSatellite(dst);

    auto satsPerPlane = tC->getSatsPerPlane();
    auto numPlanes = tC->getPlaneCount();
    auto interPlaneSpacing = tC->getInterPlaneSpacing();

    std::deque<int> route_s{src};
    std::deque<int> route_t{dst};

    // (o, i) -> i-th sat in plane o

    int i_p = srcSat->getPlane();
    int i_n = srcSat->getNumberInPlane();
    int j_p = dstSat->getPlane();
    int j_n = dstSat->getNumberInPlane();

    for (size_t time = 0; time < minHops.hHorizontal; time++) {
        int i_p_tmp;
        int j_p_tmp;
        int i_n_tmp = i_n;
        int j_n_tmp = j_n;
        switch (minHops.dir) {
            case core::SendDirection::LEFT_UP:
            case core::SendDirection::LEFT_DOWN: {
                i_p_tmp = core::fpmod(i_p - 1, numPlanes);
                j_p_tmp = core::fpmod(j_p + 1, numPlanes);
                // care for inter plane spacing between last and first plane and vice versa
                if (i_p == 0) {
                    i_n_tmp = core::fpmod(i_n_tmp - interPlaneSpacing, satsPerPlane);
                }
                if (j_p == numPlanes - 1) {
                    j_n_tmp = core::fpmod(j_n_tmp + interPlaneSpacing, satsPerPlane);
                }
            } break;
            case core::SendDirection::RIGHT_UP:
            case core::SendDirection::RIGHT_DOWN: {
                i_p_tmp = core::fpmod(i_p + 1, numPlanes);
                j_p_tmp = core::fpmod(j_p - 1, numPlanes);
                // care for inter plane spacing between last and first plane and vice versa
                if (i_p == numPlanes - 1) {
                    i_n_tmp = core::fpmod(i_n_tmp + interPlaneSpacing, satsPerPlane);
                }
                if (j_p == 0) {
                    j_n_tmp = core::fpmod(j_n_tmp - interPlaneSpacing, satsPerPlane);
                }
            } break;
        }
        int next_sat_s = tC->calculateSatelliteId(i_p_tmp, i_n_tmp);
        int next_sat_t = tC->calculateSatelliteId(j_p_tmp, j_n_tmp);
        // std::cout << "S: Check S(" << i_p << "," << i_n << ";" << tC->calculateSatelliteId(i_p, i_n) << ") + S(" << i_p_tmp << "," << i_n_tmp << ";" << next_sat_s << ")" << endl;
        // std::cout << "T: Check S(" << j_p << "," << j_n << ";" << tC->calculateSatelliteId(j_p, j_n) << ") + S(" << j_p_tmp << "," << j_n_tmp << ";" << next_sat_t << ")" << endl;
        double reward_s = abs(tC->findSatByPlaneAndNumberInPlane(i_p, i_n)->getLatitude() + tC->getSatellite(next_sat_s)->getLatitude());
        double reward_t = abs(tC->findSatByPlaneAndNumberInPlane(j_p, j_n)->getLatitude() + tC->getSatellite(next_sat_t)->getLatitude());
        // std::cout << "Reward S: " << reward_s << endl;
        // std::cout << "Reward T: " << reward_t << endl;
        if (reward_s < reward_t) {
            route_t.push_front(next_sat_t);
            j_p = j_p_tmp;
            j_n = j_n_tmp;
        } else {
            route_s.push_back(next_sat_s);
            i_p = i_p_tmp;
            i_n = i_n_tmp;
        }
    }

    VALIDATE(i_p == j_p);

    // std::cout << "Start: " << tC->calculateSatelliteId(i_p, i_n) << " in plane" << endl;

    if (minHops.hVertical == 0) {
        route_t.pop_front();
    } else {
        int numInPlane = i_n;
        for (size_t i = 1; i < minHops.hVertical; i++) {
            switch (minHops.dir) {
                case core::SendDirection::RIGHT_UP:
                case core::SendDirection::LEFT_UP: {
                    numInPlane += 1;
                } break;
                case core::SendDirection::RIGHT_DOWN:
                case core::SendDirection::LEFT_DOWN: {
                    numInPlane -= 1;
                } break;
            }
            numInPlane = core::fpmod(numInPlane, satsPerPlane);
            // std::cout << tC->calculateSatelliteId(i_p, numInPlane) << " in plane" << endl;
            route_s.push_back(tC->calculateSatelliteId(i_p, numInPlane));
        }
    }

    std::vector<int> v;

    v.insert(v.begin(), route_s.begin(), route_s.end());
    v.insert(v.end(), route_t.begin(), route_t.end());

    std::cout << "Route: " << flora::core::utils::vector::toString(v.begin(), v.end()) << endl;

    return v;
}

std::vector<int> DiscoRouting::discoRouteA2D(const core::MinHopsRes& minHops, const topologycontrol::TopologyControlBase* tC, int src, int dst) {
    ASSERT(tC != nullptr);

    // std::cout << endl << "From " << src << " to " << dst << endl;

    auto srcSat = tC->getSatellite(src);
    auto dstSat = tC->getSatellite(dst);
    auto raanDelta = tC->getRaanDelta();
    auto phaseDiff = tC->getPhaseDiff();
    auto phaseOffset = tC->getPhaseOffset();
    auto satsPerPlane = tC->getSatsPerPlane();
    auto numPlanes = tC->getPlaneCount();
    auto interPlaneSpacing = tC->getInterPlaneSpacing();

    std::deque<int> route_s{src};
    std::deque<int> route_t{dst};

    int i_p = srcSat->getPlane();
    int i_n = srcSat->getNumberInPlane();
    int j_p = dstSat->getPlane();
    int j_n = dstSat->getNumberInPlane();

    // std::cout << "i_p" << i_p << ";" << "i_n" << i_n << ";" << endl;
    // std::cout << "j_p" << j_p << ";" << "j_n" << j_n << ";" << endl;

    for (size_t time = 0; time < minHops.hVertical; time++) {
        int i_n_tmp;
        int j_n_tmp;
        switch (minHops.dir) {
            case core::SendDirection::LEFT_UP:
            case core::SendDirection::RIGHT_UP: {
                i_n_tmp = core::fpmod(i_n + 1, satsPerPlane);
                j_n_tmp = core::fpmod(j_n - 1, satsPerPlane);
            } break;
            case core::SendDirection::LEFT_DOWN:
            case core::SendDirection::RIGHT_DOWN: {
                i_n_tmp = core::fpmod(i_n - 1, satsPerPlane);
                j_n_tmp = core::fpmod(j_n + 1, satsPerPlane);
            } break;
        }
        int next_sat_s = tC->calculateSatelliteId(i_p, i_n_tmp);
        int next_sat_t = tC->calculateSatelliteId(j_p, j_n_tmp);
        double reward_s = abs(tC->findSatByPlaneAndNumberInPlane(i_p, i_n)->getLatitude() + tC->getSatellite(next_sat_s)->getLatitude());
        double reward_t = abs(tC->findSatByPlaneAndNumberInPlane(j_p, j_n)->getLatitude() + tC->getSatellite(next_sat_t)->getLatitude());
        // std::cout << "S: Check S(" << i_p << "," << i_n << ";" << tC->calculateSatelliteId(i_p, i_n) << ") -> S(" << i_p << "," << i_n_tmp << ";" << next_sat_s << "): Reward = " << reward_s << endl;
        // std::cout << "T: Check S(" << j_p << "," << j_n << ";" << tC->calculateSatelliteId(j_p, j_n) << ") -> S(" << j_p << "," << j_n_tmp << ";" << next_sat_t << "): Reward = " << reward_t << endl;
        if (reward_s < reward_t) {
            // std::cout << "  -> Select s: S(" << i_p << "," << i_n_tmp << ";" << next_sat_s << ")" << endl;
            route_s.push_back(next_sat_s);
            i_n = i_n_tmp;
        } else {
            // std::cout << "  -> Select t: S(" << j_p << "," << j_n_tmp << ";" << next_sat_t << ")" << endl;
            route_t.push_front(next_sat_t);
            j_n = j_n_tmp;
        }
    }

    // std::cout << "i_p" << i_p << ";" << "i_n" << i_n << ";" << endl;
    // std::cout << "j_p" << j_p << ";" << "j_n" << j_n << ";" << endl;

    // asserts needs to care if its the 0-th plane -> then we need to care about the phaseOffset
    if (minHops.hHorizontal == 0) {
        VALIDATE(i_n == j_n);
    } else {
        // TODO: Add validation
    }

    // std::cout << "Start: S(" << i_p << "," << i_n << ";" << tC->calculateSatelliteId(i_p, i_n) << ") in plane" << endl;

    if (minHops.hHorizontal == 0) {
        route_t.pop_front();
    } else {
        int plane = i_p;
        for (size_t i = 1; i < minHops.hHorizontal; i++) {
            switch (minHops.dir) {
                case core::SendDirection::LEFT_UP:
                case core::SendDirection::RIGHT_UP: {
                    // care for inter plane spacing between last and first plane and vice versa
                    if (plane == numPlanes - 1) {
                        i_n = core::fpmod(i_n + interPlaneSpacing, satsPerPlane);
                    }
                    plane += 1;
                } break;
                case core::SendDirection::LEFT_DOWN:
                case core::SendDirection::RIGHT_DOWN: {
                    // care for inter plane spacing between last and first plane and vice versa
                    if (plane == 0) {
                        i_n = core::fpmod(i_n - interPlaneSpacing, satsPerPlane);
                    }
                    plane -= 1;
                } break;
            }
            plane = core::fpmod(plane, numPlanes);
            // std::cout << "-> S(" << plane << "," << i_n << ";" << tC->calculateSatelliteId(plane, i_n) << ")" << endl;
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
