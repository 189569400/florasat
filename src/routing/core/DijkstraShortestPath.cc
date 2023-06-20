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
namespace dspa {

//
// DIJKSTRA FULL
//

DijkstraResultFull runDijkstraFull(const std::vector<std::vector<int>>& cost, int src) {
    std::vector<DijkstraNode> nodes = detail::runDijkstra(cost, src);
    return DijkstraResultFull{src, nodes};
}

//////////////////////////////////////////////

//
// DIJKSTRA WITH EARLY ABORT
//

DijkstraResultEarlyAbort runDijkstraEarlyAbort(const std::vector<std::vector<int>>& cost, int src, int dst) {
    std::vector<DijkstraNode> nodes = detail::runDijkstra(cost, src, dst);
    return DijkstraResultEarlyAbort{src, dst, nodes};
}

//////////////////////////////////////////////

//
// GET NEAREST ID
//

/**
 * Extracts the nearest id from the result of a dijkstra run.
 * The considered ids come from the set of potential ids.
 */
int getNearestId(const DijkstraResultFull& dijkstraResult, const std::set<int>& potentialIds) {
    // get lastSatellite with shortest distance + groundlink distance
    int nearestId = -1;
    int nearestDistance = INT_MAX;
    for (int pId : potentialIds) {
        ASSERT(pId >= 0 && pId < dijkstraResult.nodes.size());
        // add distance between sat and gs for selecting optimal last satellites
        int distance = dijkstraResult.nodes[pId].distance;
        if (distance < nearestDistance) {
            nearestDistance = distance;
            nearestId = pId;
        }
    }
    VALIDATE(nearestId != -1);
    return nearestId;
}

//////////////////////////////////////////////

//
// DIJKSTRA RECONSTRUCT PATH
//

/**
 * Constructs the path from the source to the destination, given the result of a DSPA run.
 * Can be used to construct paths to arbitrary destination ids.
 */
std::vector<int> reconstructPath(DijkstraResultFull& dijkstraResultFull, int dst) {
    return detail::reconstructPath(dijkstraResultFull.nodes, dijkstraResultFull.src, dst);
}

/**
 * Constructs the path from the source to the destination, given the result of a DSPA run.
 */
std::vector<int> reconstructPath(DijkstraResultEarlyAbort& dijkstraResultEarlyAbort) {
    return detail::reconstructPath(dijkstraResultEarlyAbort.nodes, dijkstraResultEarlyAbort.src, dijkstraResultEarlyAbort.dst);
}

//////////////////////////////////////////////

std::vector<std::vector<int>> buildShortestPathCostMatrix(int constellationSize, const std::unordered_map<int, SatelliteRoutingBase*>& constellation) {
    std::vector<std::vector<int>> cost(constellationSize, std::vector<int>(constellationSize, INT_MAX));

    for (size_t i = 0; i < constellationSize; i++) {
        const SatelliteRoutingBase* sat = constellation.at(i);
        if (sat->hasLeftSat()) {
            cost[i][sat->getLeftSatId()] = (int)round(sat->getLeftSatDistance());
        }
        if (sat->hasUpSat()) {
            cost[i][sat->getUpSatId()] = (int)round(sat->getUpSatDistance());
        }
        if (sat->hasRightSat()) {
            cost[i][sat->getRightSatId()] = (int)round(sat->getRightSatDistance());
        }
        if (sat->hasDownSat()) {
            cost[i][sat->getDownSatId()] = (int)round(sat->getDownSatDistance());
        }
    }
    return cost;
}

//
// IMPLEMENTATION DETAILS
//
namespace detail {

std::vector<DijkstraNode> runDijkstra(const std::vector<std::vector<int>>& cost, int src, int dst) {
    int size = cost.size();
    ASSERT(src >= 0 && src < size);
    ASSERT(dst == -1 || dst >= 0 && dst < size);

    std::vector<DijkstraNode> nodes(size);

    for (size_t i = 0; i < size; i++) {
        auto node = DijkstraNode();
        nodes.push_back(node);
    }

    nodes[src].distance = 0;

    for (size_t i = 0; i < size; i++) {
        int minValue = INT_MAX;
        int nearest = -1;
        for (size_t j = 0; j < size; j++) {
            if (!nodes[j].visited && nodes[j].distance < minValue) {
                minValue = nodes[j].distance;
                nearest = j;
            }
        }

        VALIDATE(nearest != -1);

        nodes[nearest].visited = true;

        // EARLY ABORT
        if (nearest == dst) break;

        for (size_t j = 0; j < size; j++) {
            if (cost[nearest][j] != INT_MAX && nodes[j].distance > nodes[nearest].distance + cost[nearest][j]) {
                nodes[j].distance = nodes[nearest].distance + cost[nearest][j];
                nodes[j].prev = nearest;
            }
        }
    }

    return nodes;
}

std::vector<int> reconstructPath(const std::vector<DijkstraNode>& nodes, int src, int dst) {
    std::deque<int> path;
    path.push_back(dst);
    int it = dst;
    while (it != src) {
        it = nodes[it].prev;
        path.push_front(it);
    }
    std::vector<int> v(std::make_move_iterator(path.begin()),
                       std::make_move_iterator(path.end()));
    return v;
}

}  // namespace detail

}  // namespace dspa
}  // namespace core
}  // namespace routing
}  // namespace flora