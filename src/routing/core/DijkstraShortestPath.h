/*
 * DijkstraShortestPath.h
 *
 * Created on: Apr 21, 2023
 *     Author: Robin Ohs
 */

#ifndef __FLORA_ROUTING_CORE_DIJKSTRASHORTESTPATH_H_
#define __FLORA_ROUTING_CORE_DIJKSTRASHORTESTPATH_H_

#include <omnetpp.h>

#include <array>
#include <deque>
#include <unordered_map>

#include "topologycontrol/data/SatelliteInfo.h"

using namespace omnetpp;
using namespace flora::topologycontrol;

#define INF 999999999

namespace flora {
namespace routing {
namespace core {

struct DijkstraResult {
    std::vector<double> distances;
    std::vector<int> prev;
};

DijkstraResult dijkstra(int src, int dst, const std::unordered_map<int, SatelliteInfo>& constellation);
DijkstraResult dijkstraEarlyAbort(int src, int dst, const std::unordered_map<int, SatelliteInfo>& constellation);

std::vector<int> reconstructPath(int src, int dest, std::vector<int> prev);

}  // namespace core
}  // namespace routing
}  // namespace flora

#endif  // __FLORA_ROUTING_CORE_DIJKSTRASHORTESTPATH_H_