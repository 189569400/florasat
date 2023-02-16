/*
 * FasterSatMobility.h
 *
 *  Created on: Feb 16, 2023
 *      Author: Robin Ohs
 */

#ifndef MOBILITY_FASTERSATMOBILITY_H_
#define MOBILITY_FASTERSATMOBILITY_H_

#include <omnetpp.h>
#include <future>
#include "mobility/SatelliteMobility.h"

namespace flora
{

    using namespace omnetpp;

    class FasterSatMobility : public cSimpleModule
    {
    public:
        FasterSatMobility();

    protected:
        virtual ~FasterSatMobility();
        virtual void initialize(int stage) override;
        virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
        virtual void handleMessage(cMessage *msg) override;
        void updatePositions();

    protected:
        /** @brief The message used for schedule mobility updates. */
        cMessage *updatePos;
        double updateIntervalPos;

    private:
        const char *POSITION = "POSITION";
    };

} // namespace flora

#endif /* MOBILITY_FASTERSATMOBILITY_H_ */
