package flora.networklayer;

import inet.networklayer.ipv4.Ipv4;

simple Ipv4Interceptor
{
    parameters:
        @display("i=block/control");
        @class(networklayer::Ipv4Interceptor);
        string networkProtocolModule;

    gates:
        output pgOut;
}
