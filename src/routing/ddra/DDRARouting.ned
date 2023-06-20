package flora.routing.ddra;

simple DDRARouting
{
    parameters:
        @class(routing::DDRARouting);
        int queueThreshold;
        double timeSliceLength @unit(s);
}