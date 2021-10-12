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

module LoRaGW like INetworkNode
{
    parameters:
        @networkNode();
        *.interfaceTableModule = default(absPath(".interfaceTable"));
        *.routingTableModule = default(routingTableType != "" ? absPath(".routingTable") : "");
        *.energySourceModule = default(exists(energyStorage) ? absPath(".energyStorage") : "");

        int numEthInterfaces = default(0);  // minimum number of ethernet interfaces
        int numWlanInterfaces = 1;
        double retardUp = default(1s) @unit(s);
        double retardRight = default(1s) @unit(s);

        bool hasIpv4 = default(true);
        string networkLayerType = default("Ipv4NetworkLayer");
        string routingTableType = default("Ipv4RoutingTable");
        int numUdpApps = default(0);
        bool hasUdp = default(firstAvailableOrEmpty("Udp") != "");
        string udpType = default(firstAvailableOrEmpty("UDP"));
        //int ID = default(0);

        LoRaGWNic.radio.antenna.mobilityModule = default("^.^.^.mobility");

        *.mobilityModule = default(exists(mobility) ? absPath(".mobility") : "");
        @display("i=device/satellite");//;r=2800,red,red
        //@display("i=abstract/cutesat");

        //@display("bgb=611,560;i=device/antennatower");
    gates:
        inout ethg[numEthInterfaces] @labels(EtherFrame-conn) @allowUnconnected;
        input radioIn[numWlanInterfaces] @directIn;
        inout left;
        inout right;
        inout up;
        inout down;

    submodules:
        interfaceTable: InterfaceTable {
            @display("p=30,26");
        }
        mobility: <default("StationaryMobility")> like IMobility {
            @display("p=24,88");
        }
        LoRaGWNic: LoRaGWNic {
            @display("p=163.56,367.728");
        }
        packetForwarder: PacketForwarder {
            @display("p=256.056,162.432;is=vl");
        }
        //SatToSatAntenna: SatToSatAntenna {
            //@display("p=140.56,367.728");
        //}
        //ISLChannel: ISLChannel{
        //}
        islPacketForwarder: ISLPacketForwarder {
        }
        eth[sizeof(ethg)]: <default("EthernetInterface")> like IWiredInterface {
            parameters:
                @display("p=340.656,442.17603,row,60;q=txQueue");
        }
        udp: <default(firstAvailableOrEmpty("Udp"))> like IUdp if hasUdp {
            parameters:
                @display("p=422.25,177.90799");
        }
        ipv4: <default("Ipv4NetworkLayer")> like INetworkLayer if hasIpv4 {
            parameters:
                @display("p=363.698,284.878;q=queue");
        }
        routingTable: <routingTableType> like IRoutingTable if routingTableType != "" {
            parameters:
                @display("p=53,250;is=s");
        }
    connections allowunconnected:
        packetForwarder.loRaPart <--> islPacketForwarder.satPart; //connection between the ISLPacketForwarder and PacketForwarder
        //islPacketForwarder.down1$o --> down$o;
        //islPacketForwarder.right$o --> right$o;
        //islPacketForwarder.left1$o --> left$o;
        //islPacketForwarder.up$o --> up$o;
        islPacketForwarder.down1$i <-- down$i;  //internal connection needed for the simulator
        islPacketForwarder.right1$i <-- right$i;
        islPacketForwarder.left1$i <-- left$i;
        islPacketForwarder.up1$i <-- up$i;
        
        
        packetForwarder.lowerLayerOut --> LoRaGWNic.upperLayerIn;
        LoRaGWNic.upperLayerOut --> packetForwarder.lowerLayerIn;
        //LoRaGWNic.right <--> right;
        packetForwarder.socketOut --> udp.appIn;
        packetForwarder.socketIn <-- udp.appOut;
        for i=0..sizeof(ethg)-1 {
            ethg[i] <--> { @display("m=s"); } <--> eth[i].phys;
            eth[i].upperLayerOut --> ipv4.ifIn;
            eth[i].upperLayerIn <-- ipv4.ifOut;
        }
        udp.ipOut --> ipv4.transportIn;
        udp.ipIn <-- ipv4.transportOut;
        //satPart.right <--> right;
        //satPart.left <--> left;
        //satPart.down <--> down;
        //satPart.up <--> up;
}
