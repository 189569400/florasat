/*
 * GsSatConnection.cc
 *
 * Created on: Feb 23, 2023
 *     Author: Robin Ohs
 */

#include "GsSatConnection.h"

namespace flora {
namespace topologycontrol {

std::string GsSatConnection::to_string() {
    std::stringstream ss;
    ss << "{";
    ss << "Conenction between GS(" << gsInfo->groundStationId << ") and SAT(" << satInfo->satelliteId << ")";
    ss << "}";
    return ss.str();
}

}  // namespace topologycontrol
}  // namespace flora
