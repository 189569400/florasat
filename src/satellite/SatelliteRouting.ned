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
import flora.routing.ForwardingTable;
import flora.satellite.SatelliteRoutingBase;
import flora.satellite.PacketHandlerRouting;
import inet.queueing.queue.DropTailQueue;

module SatelliteRouting extends SatelliteRoutingBase
{
    parameters:
        @class(satellite::SatelliteRouting);

    submodules:
        forwardingTable: ForwardingTable {
            @display("p=500,100");
        }

        packetHandler: PacketHandlerRouting {
            @display("p=700,100");
        }
        
		queue: DropTailQueue {
			@display("p=900,100");
		}

    connections allowunconnected:
        
        // queueing
        packetHandler.out --> queue.in;
        queue.out --> packetHandler.in;

        // internal connections
        packetHandler.down1  <--> down;
        packetHandler.right1  <--> right;
        packetHandler.left1  <--> left;
        packetHandler.up1  <--> up;
        for i=0..numGroundLinks-1 {
            packetHandler.groundLink1[i]  <--> groundLink[i];
        }
}