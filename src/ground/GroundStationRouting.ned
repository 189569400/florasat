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

import inet.node.inet.StandardHost;

import leosatellites.mobility.GroundStationMobility;

module GroundStationRouting extends StandardHost
{
    parameters:
        @display("p=692.48,340.83;i=device/receiverdish");
        int groundStationId;
        int numGroundLinks = default(40);

    gates:
        inout satelliteLink[numGroundLinks];

    submodules:
        gsMobility: GroundStationMobility;
        packetGenerator: PacketGenerator;

    connections allowunconnected:
        // GROUNDLINK LINKS (GS TO SATELLITE)
        for i=0..numGroundLinks-1 {
            satelliteLink[i] <--> packetGenerator.satelliteLink[i];
        }
}