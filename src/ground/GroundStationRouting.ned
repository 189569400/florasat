//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

package flora.ground;

import flora.mobility.GroundStationMobility;
import flora.traffic.UdpTrafficGenerator;
import flora.networklayer.Ipv4Interceptor;

import inet.node.ethernet.Eth1G;
import inet.node.ethernet.Eth10G;
import inet.node.ethernet.Eth100M;
import inet.common.MessageDispatcher;
import inet.linklayer.ethernet.EthernetInterface;
import inet.networklayer.common.InterfaceTable;
import inet.networklayer.ipv4.Ipv4;
import inet.transportlayer.tcp.Tcp;
import inet.node.inet.StandardHost;
import inet.networklayer.contract.INetworkLayer;

module GroundStationRouting extends StandardHost
{
    parameters:
        forwarding = true;
        hasIpv4 = true;
        numEthInterfaces = default(1);
        @class(ground::GroundStationRouting);
        @display("i=device/receiverdish");
        int numGroundLinks = default(40);
        string routerAddr;

    gates:
        inout satelliteLink[numGroundLinks] @labels(GlFrame-conn);

    submodules:
        gsMobility: GroundStationMobility {
            @display("p=125,400");
        };

        packetGenerator: PacketGenerator {
            @display("p=875,376;q=queue");
        };

        ipv4Interceptor: Ipv4Interceptor {
            parameters:
                networkProtocolModule = "^.ipv4.ip";
                @display("p=950,376");
        }

        // udpTrafficGenerator: UdpTrafficGenerator {
        //     @display("p=500,500");
        // };

    connections allowunconnected:
        packetGenerator.transportOut --> nl.in++;
        packetGenerator.transportIn <-- ipv4Interceptor.pgOut;


        //GROUNDLINK LINKS (GS TO SATELLITE)
        for i=0..numGroundLinks-1 {
            satelliteLink[i] <--> packetGenerator.satelliteLink[i];
        }

}