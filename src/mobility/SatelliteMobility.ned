package flora.mobility;

import flora.mobility.SatSGP4Mobility;

//
// Manages the SatSGP4 Mobility - extension of OS3 mobility model
//
simple SatelliteMobility extends SatSGP4Mobility
{
    parameters:
        @class(SatelliteMobility);
        bool displaySpanArea = default(false);
        int effectiveSlant = default(0);
}