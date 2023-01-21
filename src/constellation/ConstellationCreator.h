/*
 * ConstellationCreator.h
 *
 * Created on: Jan 21, 2023
 *     Author: Robin Ohs
 */

#ifndef CONSTELLATION_CONSTELLATIONCREATOR_H
#define CONSTELLATION_CONSTELLATIONCREATOR_H

#include <string.h>
#include <omnetpp.h>
#include "mobility/NoradA.h"

using namespace omnetpp;

namespace flora
{

    class ConstellationCreator : public cSimpleModule
    {
        public:
            ConstellationCreator();
        protected:
            void CreateSatellites();
            void CreateSatellite(int index, double raan, double meanAnomaly, int plane);
            virtual void initialize() override;
            // virtual void handleMessage(cMessage *msg) override;

        protected:
        /** @brief Number of satellites in the constellation. */
            int satCount;

            /** @brief Number of planes in the constellation. */
            int planeCount;

            /** @brief Number of satellites in every plane. */
            int satsPerPlane;

            /** @brief Constellation inclination, given in deg. */
            double inclination;

            /** @brief Orbit height, given in km */
            int altitude;

            /** @brief The relative spacing between satellites in adjacent planes. */
            int interPlaneSpacing;

            /** @brief The raan spread of the constellation. 180 for Walker Star, 360 for Walker Delta. */
            int raanSpread;

            /** @brief Eccentricity of the constellation. */
            double eccentricity;

            int baseYear;
            double baseDay;
            int epochYear;
            double epochDay;
    };

} // flora

#endif // CONSTELLATION_CONSTELLATIONCREATOR_H
