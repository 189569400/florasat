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

#define INF 999999999

namespace flora {
namespace routing {
namespace core {
namespace dspa {

/** @brief Struct to represent the result of a DSPA run. */
struct DijkstraResult {
    std::vector<double> distances;
    std::vector<int> prev;
};

/**
 * Runs Dijkstra shortest path from a src id with given cost matrix.
 * Dst id can be specified to enable early abort.
 */
DijkstraResult runDijkstra(int src, const std::vector<std::vector<double>> cost, int dst = -1);

/**
 * Constructs the path from the source to the destination, given the result of a DSPA run.
 */
std::vector<int> reconstructPath(int src, int dst, std::vector<int> prev);

/**
 * Creates a cost matrix given a ref to the satellite routing topology.
 * Unconnected routes are represented by 999999999.
 */
const std::vector<std::vector<double>> buildShortestPathCostMatrix(const std::unordered_map<int, SatelliteRoutingBase*>& constellation);

}  // namespace dspa
}  // namespace core
}  // namespace routing
}  // namespace flora

#endif  // __FLORA_ROUTING_CORE_DSPA_DIJKSTRASHORTESTPATH_H_