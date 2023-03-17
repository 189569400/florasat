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

import leosatellites.mobility.NoradA;
import leosatellites.mobility.SatelliteMobility;

import flora.satellite.PacketHandlerRouting;
import flora.satellite.ActivePacketConsumer;
import inet.queueing.queue.DropTailQueue;

module SatelliteRouting
{
    parameters:

    gates:
        inout groundLink[20];
        inout left;
        inout right;
        inout up;
        inout down;

    submodules:
        mobility: SatelliteMobility {
            @display("p=100,100");
        }

        NoradModule: NoradA {
            @display("p=300,100");
        }

        packetHandler: PacketHandlerRouting {
            @display("p=700,100");
        }
        
		queue: DropTailQueue {
			@display("p=900,100");
		}

        queueConsumer: ActivePacketConsumer {
            @display("p=1100,100");
        }
		
    connections allowunconnected:
        
        // queueing
        packetHandler.queueOut --> queue.in;
        queue.out --> queueConsumer.in;
        queueConsumer.out --> packetHandler.queueIn;

        // internal connections
        packetHandler.down1  <--> down;
        packetHandler.right1  <--> right;
        packetHandler.left1  <--> left;
        packetHandler.up1  <--> up;

        packetHandler.groundLink1[0]  <--> groundLink[0];
        packetHandler.groundLink1[1]  <--> groundLink[1];
        packetHandler.groundLink1[2]  <--> groundLink[2];
        packetHandler.groundLink1[3]  <--> groundLink[3];
        packetHandler.groundLink1[4]  <--> groundLink[4];
        packetHandler.groundLink1[5]  <--> groundLink[5];
        packetHandler.groundLink1[6]  <--> groundLink[6];
        packetHandler.groundLink1[7]  <--> groundLink[7];
        packetHandler.groundLink1[8]  <--> groundLink[8];
        packetHandler.groundLink1[9]  <--> groundLink[9];
        packetHandler.groundLink1[10]  <--> groundLink[10];
        packetHandler.groundLink1[11]  <--> groundLink[11];
        packetHandler.groundLink1[12]  <--> groundLink[12];
        packetHandler.groundLink1[13]  <--> groundLink[13];
        packetHandler.groundLink1[14]  <--> groundLink[14];
        packetHandler.groundLink1[15]  <--> groundLink[15];
        packetHandler.groundLink1[16]  <--> groundLink[16];
        packetHandler.groundLink1[17]  <--> groundLink[17];
        packetHandler.groundLink1[18]  <--> groundLink[18];
        packetHandler.groundLink1[19]  <--> groundLink[19];

}