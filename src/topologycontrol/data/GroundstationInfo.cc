/*
 * GroundstationInfo.cc
 *
 * Created on: Feb 06, 2023
 *     Author: Robin Ohs
 */

#include "GroundstationInfo.h"

namespace flora {
namespace topologycontrol {

std::string GroundstationInfo::to_string() {
    std::stringstream ss;
    ss << "{";
    ss << "\"groundStationId\": " << groundStationId << ",";
    ss << "\"groundStation\": " << groundStation << ",";
    ss << "\"satellites\": "
       << "[";
    for (int satellite : satellites) {
        ss << satellite << ",";
    }
    ss << "],";
    ss << "}";
    return ss.str();
}

}  // namespace topologycontrol
}  // namespace flora
