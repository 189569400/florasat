/*
 * FasterSatMobility.h
 *
 *  Created on: Feb 16, 2023
 *      Author: Robin Ohs
 */

#ifndef __FLORA_MOBILITY_FASTERSATMOBILITY_H_
#define __FLORA_MOBILITY_FASTERSATMOBILITY_H_

#include <omnetpp.h>

#include "core/Timer.h"
#include "inet/common/clock/ClockUserModuleMixin.h"
#include "mobility/NoradA.h"
#include "mobility/SatelliteMobility.h"

namespace flora {
namespace mobility {

using SatMobVector = std::vector<inet::SatelliteMobility *>;

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
    /** @brief Loads the SatelliteMobility of all satellites. */
    SatMobVector loadSatMobilities();

   protected:
    /** @brief The simulation time interval used to regularly signal mobility state changes. */
    cPar *updateIntervalParameter = nullptr;
    inet::ClockEvent *updateTimer = nullptr;

   private:
    const char *POSITION = "POSITION";
    /** @brief Used to store sat mobilities. */
    SatMobVector satMobVector;
};

}  // namespace mobility
}  // namespace flora

#endif /* __FLORA_MOBILITY_FASTERSATMOBILITY_H_ */
