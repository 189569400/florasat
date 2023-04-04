/*
 * FasterSatMobility.h
 *
 *  Created on: Feb 16, 2023
 *      Author: Robin Ohs
 */

#ifndef __FLORA_MOBILITY_FASTERSATMOBILITY_H_
#define __FLORA_MOBILITY_FASTERSATMOBILITY_H_

#include <omnetpp.h>

#include <future>

#include "inet/common/clock/ClockUserModuleMixin.h"
#include "mobility/SatelliteMobility.h"

namespace flora {
namespace mobility {

class FasterSatMobility : public inet::ClockUserModuleMixin<omnetpp::cSimpleModule> {
   public:
    FasterSatMobility();

   protected:
    virtual ~FasterSatMobility();
    virtual void initialize(int stage) override;
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    virtual void handleMessage(cMessage *msg) override;
    void updatePositions();
    /** @brief Schedules the update timer that will update the topology state.*/
    void scheduleUpdate();

   protected:
    /**
     * @brief The simulation time interval used to regularly signal mobility state changes.
     *
     * The 0 value turns off the signal.
     */
    cPar *updateIntervalParameter = nullptr;
    inet::ClockEvent *updateTimer = nullptr;

   private:
    const char *POSITION = "POSITION";
};

}  // namespace mobility
}  // namespace flora

#endif /* __FLORA_MOBILITY_FASTERSATMOBILITY_H_ */
