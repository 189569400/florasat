/*
 * DijkstraShortestPath.cc
 *
 * Created on: Apr 21, 2023
 *     Author: Robin Ohs
 */

#include "DijkstraShortestPath.h"

namespace flora {
namespace routing {
namespace core {

DijkstraResult runDijkstra(int src, const std::vector<std::vector<double>> cost, int dst, bool earlyAbort) {
    int size = cost.size();
    ASSERT(src >= 0 && src < size);
    ASSERT(!earlyAbort || dst >= 0 && dst < size);

    std::vector<double> distance;
    std::vector<int> prev;
    std::vector<bool> visited;

    for (size_t i = 0; i < size; i++) {
        distance.push_back(INF);
        prev.push_back(-1);
        visited.push_back(false);
    }

    distance[src] = 0;

    for (size_t i = 0; i < size; i++) {
        int minValue = INF;
        int nearest = -1;
        for (size_t j = 0; j < size; j++) {
            if (!visited[j] && distance[j] < minValue) {
                minValue = distance[j];
                nearest = j;
            }
        }
        ASSERT(nearest != -1);
        visited[nearest] = true;

        // EARLY ABORT
        if (earlyAbort && nearest == dst) break;

        for (size_t j = 0; j < size; j++) {
            if (cost[nearest][j] != INF && distance[j] > distance[nearest] + cost[nearest][j]) {
                distance[j] = distance[nearest] + cost[nearest][j];
                prev[j] = nearest;
            }
        }
    }

    std::vector<double> vDistance;
    for (size_t i = 0; i < size; i++) {
        vDistance.push_back(distance[i]);
    }

    std::vector<int> vPrev;
    for (size_t i = 0; i < size; i++) {
        vPrev.push_back(prev[i]);
    }

    return DijkstraResult{vDistance, vPrev};
}

std::vector<int> reconstructPath(int src, int dest, std::vector<int> prev) {
    std::deque<int> path;
    path.push_back(dest);
    int it = dest;
    while (it != src) {
        it = prev[it];
        path.push_front(it);
    }
    std::vector<int> v(std::make_move_iterator(path.begin()),
                       std::make_move_iterator(path.end()));
    return v;
}

const std::vector<std::vector<double>> buildCostMatrix(const std::unordered_map<int, SatelliteRoutingBase*>& constellation) {
    std::vector<std::vector<double>> cost;
    size_t size = constellation.size();

    for (size_t i = 0; i < size; i++) {
        std::vector<double> v;
        for (size_t j = 0; j < size; j++) {
            v.push_back(INF);
        }
        const SatelliteRoutingBase* sat = constellation.at(i);
        if (sat->hasLeftSat()) {
            v[sat->getLeftSatId()] = sat->getLeftSatDistance();
        }
        if (sat->hasUpSat()) {
            v[sat->getUpSatId()] = sat->getUpSatDistance();
        }
        if (sat->hasRightSat()) {
            v[sat->getRightSatId()] = sat->getRightSatDistance();
        }
        if (sat->hasDownSat()) {
            v[sat->getDownSatId()] = sat->getDownSatDistance();
        }
#ifndef NDEBUG
        EV_DEBUG << i << " costs: ";
        auto begin = v.begin();
        auto end = v.end();
        bool first = true;
        for (; begin != end; begin++) {
            if (!first)
                EV_DEBUG << ", ";
            auto val = std::lround(*begin);
            auto print = val == INF ? "INF" : std::to_string(val);
            EV_DEBUG << print;
            first = false;
        }
        EV_DEBUG << endl;
#endif
        cost.emplace_back(v);
    }
    return cost;
}

}  // namespace core
}  // namespace routing
}  // namespace flora