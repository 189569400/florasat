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
import inet.networklayer.common.InterfaceTable;
import inet.linklayer.acking.AckingWirelessInterface;

import leosatellites.mobility.NoradA;
import leosatellites.mobility.SatelliteMobility;

import flora.LoRa.LoRaGWNic;
import flora.satellite.PacketHandlerWired;

module SatelliteWired like INetworkNode
{
    parameters:
        @networkNode();
        *.interfaceTableModule = default(absPath(".interfaceTable"));
        *.routingTableModule = default(routingTableType != "" ? absPath(".routingTable") : "");
        *.energySourceModule = default(exists(energyStorage) ? absPath(".energyStorage") : "");
        @display("bgb=363,251");

    gates:
        inout groundLink;
        inout left;
        inout right;
        inout up;
        inout down;

    submodules:
        mobility: SatelliteMobility {
            @display("p=54,56");
        }

        NoradModule: NoradA {
            @display("p=153,56");
        }

        LoRaGWNic: LoRaGWNic {
            @display("p=54,161");
        }

        interfaceTable: InterfaceTable {
            @display("p=270,56");
        }

        packetHandler: PacketHandlerWired {
            @display("p=181,161");
        }

    connections allowunconnected:
        
        //internal connection needed for the simulator
        packetHandler.down  <--> down;
        packetHandler.right  <--> right;
        packetHandler.left  <--> left;
        packetHandler.up  <--> up;
        
        // LoRa packet to and from the LoRaGWNic and the PacketHandler
        LoRaGWNic.upperLayerIn <-- packetHandler.lowerLayerLoRaOut;
        LoRaGWNic.upperLayerOut --> packetHandler.lowerLayerLoRaIn;

        // packet to and from the GroundStation and the PacketHandler
        // uhf radio module to be implemented
        groundLink <--> packetHandler.lowerLayerGS;
}