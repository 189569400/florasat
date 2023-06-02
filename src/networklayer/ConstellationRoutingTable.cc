/*
 * ConstellationRoutingTable.cc
 *
 *  Created on: Apr 24, 2023
 *      Author: Robin Ohs
 */

#include "ConstellationRoutingTable.h"

namespace flora {
namespace networklayer {

Define_Module(ConstellationRoutingTable);

void ConstellationRoutingTable::initialize(int stage) {
    if (stage == INITSTAGE_LOCAL) {
        int numGs = getSystemModule()->getSubmoduleVectorSize("groundStation");
        for (size_t i = 0; i < numGs; i++) {
            cModule* pg = getSystemModule()->getSubmodule("groundStation", i)->getSubmodule("packetGenerator");
            auto serving = pg->par("serving").stringValue();
            cStringTokenizer tokenizer(serving);
            const char* token;
            while ((token = tokenizer.nextToken()) != nullptr) {
                L3Address result = L3Address(token);
                addEntry(result, i);
            }
        }
    }
}

void ConstellationRoutingTable::addEntry(L3Address address, int gsId) {
    if (entries.find(address) != entries.end()) {
        error("Error in ConstellationRoutingTable::addEntry: Entry for %s already present.", address.str().c_str());
    }
    entries.emplace(address, gsId);
}

void ConstellationRoutingTable::removeEntry(L3Address address, int gsId) {
    if (entries.find(address) == entries.end()) {
        error("Error in ConstellationRoutingTable::addEntry: Entry for %s not present.", address.str().c_str());
    }
    entries.erase(address);
}

L3Address ConstellationRoutingTable::getAddressOfGroundstation(int gsId) {
    for (auto t : entries) {
        if (t.second == gsId) {
            return t.first;
        }
    }
    throw new cRuntimeError("Error in ConstellationRoutingTable::getGroundstationFromAddress: Entry for %d not present.", gsId);
}

int ConstellationRoutingTable::getGroundstationFromAddress(L3Address address) {
    if (entries.find(address) == entries.end()) {
        error("Error in ConstellationRoutingTable::getAddressOfGroundstation: Entry for %s not present.", address.str().c_str());
    }
    return entries.at(address);
}

}  // namespace networklayer
}  // namespace flora
