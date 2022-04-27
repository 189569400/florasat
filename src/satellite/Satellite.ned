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

import leosatellites.mobility.NoradA;
import leosatellites.mobility.SatelliteMobility;

import flora.LoRa.LoRaGWNic;
import flora.LoRaApp.SimpleLoRaApp;
import flora.satellite.PacketHandler;
import flora.satellite.ISLNic;

module Satellite like INetworkNode
{
    parameters:
        *.interfaceTableModule = default(absPath(".interfaceTable"));
        *.routingTableModule = default(routingTableType != "" ? absPath(".routingTable") : "");
        *.energySourceModule = default(exists(energyStorage) ? absPath(".energyStorage") : "");
        @display("bgb=367,206");
        @networkNode();
    gates:
        inout groundLink;

    submodules:
        mobility: SatelliteMobility {
            @display("p=46,40");
        }

        NoradModule: NoradA {
            @display("p=136,40");
        }

        LoRaGWNic: LoRaGWNic {
            @display("p=61,131");
        }
        
        interfaceTable: InterfaceTable {
            @display("p=135.36,69.936005");
        }

        ISLNic: ISLAckingWirelessInterface {
            @display("p=318,131");
        }

        packetHandler: PacketHandler {
            @display("p=186,131");
        }

    connections allowunconnected:
        // LoRa packet to and from the LoRaGWNic and the PacketHandler
        LoRaGWNic.upperLayerIn <-- packetHandler.lowerLayerLoRaOut;
        LoRaGWNic.upperLayerOut --> packetHandler.lowerLayerLoRaIn;

        // packet to and from the ISLNic and the PacketHandler
        ISLNic.upperLayerIn <-- packetHandler.lowerLayerISLOut;
        ISLNic.upperLayerOut --> packetHandler.lowerLayerISLIn;

		// packet to and from the GroundStation and the PacketHandler
		// uhf radio module to be implemented
		groundLink  <--> packetHandler.lowerLayerGS;
}




