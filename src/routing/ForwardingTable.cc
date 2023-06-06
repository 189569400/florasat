/*
 * ForwardingTable.cc
 *
 *  Created on: Jun 01, 2023
 *      Author: Robin Ohs
 */

#include "ForwardingTable.h"

namespace flora {

Define_Module(ForwardingTable);

ForwardingTable::ForwardingTable() {
}

ForwardingTable::~ForwardingTable() {
}

cModule* ForwardingTable::getHostModule() {
    return findContainingNode(this);
}

void ForwardingTable::printForwardingTable() const {
    Enter_Method("printForwardingTable");
    EV << "-- Forwarding table --\n";
    EV << ::inet::utils::stringf("%-10s %-10s\n",
                                 "Destination", "Out");

    for (auto const& x : routes) {
        auto dst = x.first;
        auto out = to_string(x.second);
        EV << ::inet::utils::stringf("%-10d %-10s\n", dst, out.c_str());
    }
    EV << "\n";
}

void ForwardingTable::setRoute(int gsId, isldirection::ISLDirection dir) {
    Enter_Method("setRoute", gsId, dir);
    routes.emplace(gsId, dir);
}

void ForwardingTable::removeRoute(int gsId) {
    Enter_Method("removeRoute", gsId);
    routes.erase(gsId);
}

void ForwardingTable::clearRoutes() {
    Enter_Method("clearRoutes");
    routes.clear();
}

isldirection::ISLDirection ForwardingTable::getNextHop(int gsId) {
    Enter_Method("getNextHop", gsId);
    return routes.at(gsId);
}

}  // namespace flora