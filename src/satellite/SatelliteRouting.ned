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

module SatelliteRouting
{
    parameters:

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

        packetHandler: PacketHandlerRouting {
            @display("p=181,161");
        }

    connections allowunconnected:

        // internal connections
        packetHandler.down1  <--> down;
        packetHandler.right1  <--> right;
        packetHandler.left1  <--> left;
        packetHandler.up1  <--> up;
        packetHandler.groundLink1  <--> groundLink;

}