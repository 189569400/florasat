/*
 * DijkstraShortestPath.h
 *
 * Created on: Apr 21, 2023
 *     Author: Robin Ohs
 */

#ifndef __FLORA_ROUTING_CORE_DSPA_DIJKSTRASHORTESTPATH_H_
#define __FLORA_ROUTING_CORE_DSPA_DIJKSTRASHORTESTPATH_H_

#include <omnetpp.h>

#include <array>
#include <deque>
#include <unordered_map>

#include "core/utils/VectorUtils.h"
#include "satellite/SatelliteRoutingBase.h"

using namespace omnetpp;
using namespace flora::satellite;

namespace flora {
namespace routing {
namespace core {
namespace dspa {

struct DijkstraNode {
    bool visited = false;
    int distance = INT_MAX;
    int prev = -1;
};

//
// DIJKSTRA FULL
//

/** @brief Struct to represent the result of a DSPA run with early abort. */
struct DijkstraResultFull {
    int src;
    std::vector<DijkstraNode> nodes;
};

/** @brief Runs Dijkstra shortest path from src id with given cost matrix to all other. */
DijkstraResultFull runDijkstraFull(const std::vector<std::vector<int>>& cost, int src);

//////////////////////////////////////////////

//
// DIJKSTRA WITH EARLY ABORT
//

/** @brief Struct to represent the result of a DSPA run with early abort. */
struct DijkstraResultEarlyAbort {
    int src;
    int dst;
    std::vector<DijkstraNode> nodes;
};

/** @brief Runs Dijkstra shortest path from src id with given cost matrix to all other but early abort if dst is reached. */
DijkstraResultEarlyAbort runDijkstraEarlyAbort(const std::vector<std::vector<double>>& cost, int src, int dst);

//////////////////////////////////////////////

//
// GET NEAREST ID
//

/**
 * Extracts the nearest id from the result of a dijkstra run.
 * The considered ids come from the set of potential ids.
 */
int getNearestId(const DijkstraResultFull& dijkstraResult, const std::set<int>& potentialIds);

//////////////////////////////////////////////

//
// DIJKSTRA RECONSTRUCT PATH
//

/**
 * Constructs the path from the source to the destination, given the result of a DSPA run.
 * Can be used to construct paths to arbitrary destination ids.
 */
std::vector<int> reconstructPath(DijkstraResultFull& dijkstraResultFull, int dst);

/**
 * Constructs the path from the source to the destination, given the result of a DSPA run.
 */
std::vector<int> reconstructPath(DijkstraResultEarlyAbort& dijkstraResultEarlyAbort);

//////////////////////////////////////////////

//
// BUILD COST MATRIX
//

/**
 * Creates a cost matrix given a ref to the satellite routing topology.
 * Unconnected routes are represented by 999999999.
 */
std::vector<std::vector<int>> buildShortestPathCostMatrix(int constellationSize, const std::unordered_map<int, SatelliteRoutingBase*>& constellation);

//////////////////////////////////////////////

namespace detail {

std::vector<DijkstraNode> runDijkstra(const std::vector<std::vector<int>>& cost, int src, int dst = -1);
std::vector<int> reconstructPath(const std::vector<DijkstraNode>& prev, int src, int dst);

}  // namespace detail

}  // namespace dspa
}  // namespace core
}  // namespace routing
}  // namespace flora

#endif  // __FLORA_ROUTING_CORE_DSPA_DIJKSTRASHORTESTPATH_H_