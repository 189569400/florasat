/*
 * ConstellationCreator.cc
 *
 * Created on: Jan 21, 2023
 *     Author: Robin Ohs
 */

#include "ConstellationCreator.h"

namespace flora
{
    Define_Module(ConstellationCreator);

    ConstellationCreator::ConstellationCreator() : planeCount(0),
                                                   inclination(0.0),
                                                   altitude(0),
                                                   interPlaneSpacing(0),
                                                   baseYear(0),
                                                   baseDay(0.0),
                                                   epochYear(0),
                                                   epochDay(0.0),
                                                   eccentricity(0.0),
                                                   raanSpread(0),
                                                   satsPerPlane(0),
                                                   satCount(0)
    {
    }

    void ConstellationCreator::initialize()
    {
        EV << "Create Constellation" << endl;

        planeCount = par("planeCount");
        inclination = par("inclination");
        altitude = par("altitude");
        interPlaneSpacing = par("interPlaneSpacing");
        baseYear = par("baseYear");
        baseDay = par("baseDay");
        epochYear = par("epochYear");
        epochDay = par("epochDay");
        eccentricity = par("eccentricity");
        raanSpread = par("raanSpread");

        // get satellite number and compute sats_per_plane
        satCount = getParentModule()->getSubmoduleVectorSize("loRaGW");
        if (satCount % planeCount != 0)
        {
            error("Error in ConstellationCreator::initialize(): Sat count (%d) must be divisble by plane count (%d).", satCount, planeCount);
        }
        satsPerPlane = satCount / planeCount;

        EV << "Orbital params: "
           << "satCount: " << satCount << "; "
           << "planeCount: " << planeCount << "; "
           << "inclination: " << inclination << "; "
           << "altitude: " << altitude << "; "
           << "interPlaneSpacing: " << interPlaneSpacing << "; "
           << "baseYear: " << baseYear << "; "
           << "baseDay: " << baseDay << "; "
           << "epochYear: " << epochYear << "; "
           << "epochDay: " << epochDay << "; "
           << "eccentricity: " << eccentricity << "; "
           << "raanSpread: " << raanSpread << "; "
           << endl;

        CreateSatellites();
    }

    void ConstellationCreator::CreateSatellites()
    {
        double meanAnomalyDelta = 360.0 / satsPerPlane;
        // double angularMeasurePerSlot = (360.0 / satCount) - eccentricity * sin(angularMeasurePerSlot);
        double angularMeasurePerSlot = (360.0 / satCount);
        double raanDelta = raanSpread / planeCount;

        // iterate over planes
        for (size_t plane = 0; plane < planeCount; plane++)
        {
            // create plane satellites
            double raan = raanDelta * plane;
            for (size_t planeSat = 0; planeSat < satsPerPlane; planeSat++)
            {
                int index = planeSat + plane * satsPerPlane;
                double meanAnomaly = std::fmod(planeSat * meanAnomalyDelta + plane * angularMeasurePerSlot * interPlaneSpacing, 360.0);
                CreateSatellite(index, raan, meanAnomaly, plane);
            }
        }
    }

    void ConstellationCreator::CreateSatellite(int index, double raan, double meanAnomaly, int plane)
    {
        cModule *parent = getParentModule();
        if (parent == nullptr)
        {
            error("Error in ConstellationCreator::CreateSatellite(): parent is nullptr");
        }
        cModule *sat = parent->getSubmodule("loRaGW", index);
        if (sat == nullptr)
        {
            error("Error in ConstellationCreator::CreateSatellite(): sat(%d) is nullptr", index);
        }
        NoradA *noradModule = check_and_cast<NoradA *>(sat->getSubmodule("NoradModule"));
        if (noradModule == nullptr)
        {
            error("Error in ConstellationCreator::CreateSatellite(): noradModule of sat(%d) is nullptr", index);
        }
        noradModule->par("satIndex").setIntValue(index);
        noradModule->par("baseYear").setIntValue(baseYear);
        noradModule->par("baseDay").setDoubleValue(baseDay);
        noradModule->par("planes").setIntValue(planeCount);
        noradModule->par("satPerPlane").setIntValue(satsPerPlane);
        noradModule->par("epochYear").setIntValue(epochYear);
        noradModule->par("epochDay").setDoubleValue(epochDay);
        noradModule->par("eccentricity").setDoubleValue(eccentricity);
        noradModule->par("inclination").setDoubleValue(inclination);
        noradModule->par("altitude").setDoubleValue(altitude);
        noradModule->par("raan").setDoubleValue(raan);
        noradModule->par("meanAnomaly").setDoubleValue(meanAnomaly);
        noradModule->initializeMobility(simTime());
    }

} // flora