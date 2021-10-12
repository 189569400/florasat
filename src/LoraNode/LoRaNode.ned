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

import inet.mobility.static.StationaryMobility;
import inet.networklayer.common.InterfaceTable;
import flora.LoRa.LoRaNic;
import flora.LoRaApp.SimpleLoRaApp;
//import inet.networklayer.contract.IRoutingTable;
//import inet.networklayer.contract.INetworkLayer;


module LoRaNode
{
    parameters:
        string deploymentType = default("");
        double maxGatewayDistance = default(320.0);
        double gatewayX = default(320.0);
        double gatewayY = default(320.0);
        //bool hasIpv4 = default(true);
        @networkNode();
        *.interfaceTableModule = default(absPath(".interfaceTable"));
        //*.routingTableModule = default(routingTableType != "" ? absPath(".routingTable") : "");
        //string routingTableType = default("Ipv4RoutingTable");
        //string networkLayerType = default("Ipv4NetworkLayer");
        @display("bgb=297,313;i=device/accesspoint;is=vs");
    submodules:
        interfaceTable: InterfaceTable {
            @display("p=30,26");
        }
        mobility: StationaryMobility {
            @display("p=24,88");
        }
        LoRaNic: LoRaNic {
            @display("p=137,239");
        }
        SimpleLoRaApp: SimpleLoRaApp {
            @display("p=137,32");
        }
//        routingTable: <routingTableType> like IRoutingTable if routingTableType != "" {
//            parameters:
//                @display("p=37,200;is=s");
//        }
//        ipv4: <default("Ipv4NetworkLayer")> like INetworkLayer if hasIpv4 {
//            parameters:
//                @display("p=363.698,284.878;q=queue");
//        }
        
        
    connections allowunconnected:

        SimpleLoRaApp.appOut --> LoRaNic.upperLayerIn;
        LoRaNic.upperLayerOut --> SimpleLoRaApp.appIn;
}
