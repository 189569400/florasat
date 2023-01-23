package flora.constellation;

simple ConstellationCreator
{
    parameters:
        // satCount gets defined by nrOfGW
        // int satCount;
        int planeCount;
        double inclination = default(86.4);
        int altitude = default(780);
        int interPlaneSpacing = default(1);
        int baseYear = default(20);
        double baseDay = default(0.8292315);
        int epochYear = default(20);
        double epochDay = default(0.8194444);
        double eccentricity = default(0.000001);
        int raanSpread = default(180);
}