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

package flora.satellite;

import inet.node.inet.INetworkNode;

import leosatellites.mobility.NoradA;
import leosatellites.mobility.SatelliteMobility;

import flora.LoRa.LoRaGWNic;
import flora.LoRaApp.SimpleLoRaApp;
import flora.satellite.PacketHandler;
import flora.satellite.ISLNic;

module Satellite like INetworkNode
{
    parameters:
        int numEthInterfaces = default(0);
        int numWlanInterfaces = 3;
    
    gates:
        inout ethg @labels(EtherFrame-conn) @allowUnconnected;
        input radioIn[numWlanInterfaces] @directIn;
        
    submodules:
        mobility: SatelliteMobility {
            @display("p=71,158");
        }
        
        NoradModule: NoradA {
            @display("p=71,300");
        }
        
        LoRaGWNic: LoRaGWNic {
            @display("p=202,345");
        }
        
        ISLNic : ISLNic {
        }
        
        packetHandler: PacketHandler {
        }
        
    connections allowunconnected:
        // direct LoRa packet to and from the Nic and the PacketHandler
        LoRaGWNic.upperLayerIn  <-- packetHandler.lowerLayerLoRaOut;
        LoRaGWNic.upperLayerOut --> packetHandler.lowerLayerLoRaIn;
        
        //connection between the ISLPacketForwarder and PacketForwarder
        packetForwarder.loRaPart <--> islPacketForwarder.satPart;

		//internal connection needed for the simulator
        islPacketForwarder.down1  <--> down;
        islPacketForwarder.right1 <--> right;
        islPacketForwarder.left1  <--> left;
        islPacketForwarder.up1    <--> up;
        
        //internal connection needed for the simulator
        // packetHandler.islLeft  <--> left;
        // packetHandler.islRight <--> right;
        // packetHandler.islDown  <--> down;
        // packetHandler.islUp    <--> up;
        
        // prepare and send UDP packet to network server
        packetForwarder.socketOut --> udp.appIn;
        packetForwarder.socketIn  <-- udp.appOut;

        udp.ipOut --> ipv4.transportIn;
        udp.ipIn  <-- ipv4.transportOut;
        
        ipv4.ifOut --> md.in++;  // IP sends datagrams
        ipv4.ifIn  <-- md.out++; // IP receives datagrams
        
        md.out++ --> eth.upperLayerIn;
        md.in++  <-- eth.upperLayerOut;

		ethg <--> { @display("m=s"); } <--> eth.phys;
        
       
}




