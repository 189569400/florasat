/*
 * ForwardingTable.h
 *
 *  Created on: Jun 01, 2023
 *      Author: Robin Ohs
 */

#ifndef __FLORA_ROUTING_FORWARDINGTABLE_H_
#define __FLORA_ROUTING_FORWARDINGTABLE_H_

#include <omnetpp.h>

#include <unordered_map>

#include "core/ISLDirection.h"
#include "inet/common/INETDefs.h"
#include "inet/common/INETUtils.h"
#include "inet/common/ModuleAccess.h"

using namespace omnetpp;
using namespace inet;
using namespace flora::core;

namespace flora {

class ForwardingTable : public cSimpleModule {
   public:
    ForwardingTable();
    ~ForwardingTable();

    /** @brief Prints the forwarding table for debugging purposes. */
    void printForwardingTable() const;

    /** @brief Returns the module that contains the ForwardingTable. Nullptr if none.*/
    cModule *getHostModule();

    void setRoute(int gsId, isldirection::ISLDirection dir);
    void removeRoute(int gsId);
    void clearRoutes();

    isldirection::ISLDirection getNextHop(int gsId);

   protected:
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }

   private:
    std::unordered_map<int, isldirection::ISLDirection> routes;
};

}  // namespace flora

#endif  // __FLORA_ROUTING_FORWARDINGTABLE_H_
