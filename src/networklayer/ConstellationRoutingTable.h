/*
 * ConstellationRoutingTable.h
 *
 *  Created on: Apr 24, 2023
 *      Author: Robin Ohs
 */

#ifndef __FLORA_NETWORKLAYER_CONSTELLATIONROUTINGTABLE_H_
#define __FLORA_NETWORKLAYER_CONSTELLATIONROUTINGTABLE_H_

#include <omnetpp.h>

#include <map>

#include "inet/networklayer/common/L3Address.h"

using namespace omnetpp;
using namespace inet;

namespace flora {
namespace networklayer {

class ConstellationRoutingTable : public cSimpleModule {
   protected:
    std::map<L3Address, int> entries;

   public:
    void addEntry(L3Address address, int gsId);
    void removeEntry(L3Address address, int gsId);
    L3Address getAddressOfGroundstation(int gsId);
    int getGroundstationFromAddress(L3Address address);

   protected:
    virtual void initialize(int stage) override;
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
};

}  // namespace networklayer
}  // namespace flora

#endif  // __FLORA_NETWORKLAYER_CONSTELLATIONROUTINGTABLE_H_
