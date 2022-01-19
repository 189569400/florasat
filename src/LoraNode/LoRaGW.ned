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

package flora.LoraNode;

import inet.node.inet.INetworkNode;
import inet.mobility.static.StationaryMobility;
import inet.mobility.contract.IMobility;
import inet.networklayer.common.InterfaceTable;
import inet.networklayer.contract.INetworkLayer;
import inet.linklayer.loopback.LoopbackInterface;
import flora.LoRa.LoRaGWNic;
import flora.LoRaApp.SimpleLoRaApp;
import flora.LoRa.PacketForwarder;
import flora.LoRa.ISLPacketForwarder;
//import flora.LoRa.SatToSatAntenna;
import inet.linklayer.contract.IWiredInterface;
import inet.applications.contract.IApp;
import inet.transportlayer.contract.IUdp;
import inet.networklayer.contract.IRoutingTable;
import flora.LoRa.ISLChannel;
import inet.common.MessageDispatcher;

module LoRaGW like INetworkNode
{
    parameters:
        @networkNode();
        *.interfaceTableModule = default(absPath(".interfaceTable"));
        *.routingTableModule = default(routingTableType != "" ? absPath(".routingTable") : "");
        *.energySourceModule = default(exists(energyStorage) ? absPath(".energyStorage") : "");

        int numEthInterfaces = default(0);  // minimum number of ethernet interfaces
        int numWlanInterfaces = 1;
        double retardUp @mutable @unit(s) = default(1s);
        double retardRight @mutable @unit(s) = default(1s);

        bool hasIpv4 = default(true);
        string networkLayerType = default("Ipv4NetworkLayer");
        string routingTableType = default("Ipv4RoutingTable");
        int numUdpApps = default(0);
        bool hasUdp = default(firstAvailableOrEmpty("Udp") != "");
        string udpType = default(firstAvailableOrEmpty("UDP"));

        LoRaGWNic.radio.antenna.mobilityModule = default("^.^.^.mobility");

        *.mobilityModule = default(exists(mobility) ? absPath(".mobility") : "");
        @display("i=device/satellite,red,90;is=s");//;r=2800,red,red

    gates:
        inout ethg @labels(EtherFrame-conn) @allowUnconnected;
        input radioIn[numWlanInterfaces] @directIn;
        inout left;
        inout right;
        inout up;
        inout down;

    submodules:

        md: MessageDispatcher {
            @display("p=403,259");
        }
        interfaceTable: InterfaceTable {
            @display("p=71,77");
        }
        mobility: <default("StationaryMobility")> like IMobility {
            @display("p=71,158");
        }
        LoRaGWNic: LoRaGWNic {
            @display("p=202,345");
        }
        packetForwarder: PacketForwarder {
            @display("p=202,185;is=vl");
        }
        islPacketForwarder: ISLPacketForwarder {
            @display("p=202,77");
        }
        eth: <default("EthernetInterface")> like IWiredInterface {
            parameters:
                @display("p=403,345,row,60;q=txQueue");
        }
        udp: <default(firstAvailableOrEmpty("Udp"))> like IUdp if hasUdp {
            parameters:
                @display("p=321,185");
        }
        ipv4: <default("Ipv4NetworkLayer")> like INetworkLayer if hasIpv4 {
            parameters:
                @display("p=403,185;q=queue");
        }
        routingTable: <routingTableType> like IRoutingTable if routingTableType != "" {
            parameters:
                @display("p=71,235;is=s");
        }
    connections allowunconnected:
        packetForwarder.loRaPart <--> islPacketForwarder.satPart; //connection between the ISLPacketForwarder and PacketForwarder

        islPacketForwarder.down1$i <-- down$i;  //internal connection needed for the simulator
        islPacketForwarder.right1$i <-- right$i;
        islPacketForwarder.left1$i <-- left$i;
        islPacketForwarder.up1$i <-- up$i;

        packetForwarder.lowerLayerOut --> LoRaGWNic.upperLayerIn;
        LoRaGWNic.upperLayerOut --> packetForwarder.lowerLayerIn;
        //LoRaGWNic.right <--> right;
        packetForwarder.socketOut --> udp.appIn;
        packetForwarder.socketIn <-- udp.appOut;

        ethg <--> { @display("m=s"); } <--> eth.phys;
        md.out++ --> eth.upperLayerIn;
        md.in++ <-- eth.upperLayerOut;
        ipv4.ifOut --> md.in++; // IP sends datagrams
        ipv4.ifIn <-- md.out++; // IP receives datagrams

        udp.ipOut --> ipv4.transportIn;
        udp.ipIn <-- ipv4.transportOut;
}
