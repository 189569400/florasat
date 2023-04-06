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

import leosatellites.mobility.GroundStationMobility;

module GroundStationRouting
{
    parameters:
        int groundStationId;

    gates:
        inout satelliteLink[40];

    submodules:
        mobility: GroundStationMobility {
            @display("p=71,158");
        }

        packetGenerator: PacketGenerator {
            @display("p=71,158");
        }

    connections allowunconnected:
        satelliteLink[0] <--> packetGenerator.satelliteLink[0];
        satelliteLink[1] <--> packetGenerator.satelliteLink[1];
        satelliteLink[2] <--> packetGenerator.satelliteLink[2];
        satelliteLink[3] <--> packetGenerator.satelliteLink[3];
        satelliteLink[4] <--> packetGenerator.satelliteLink[4];
        satelliteLink[5] <--> packetGenerator.satelliteLink[5];
        satelliteLink[6] <--> packetGenerator.satelliteLink[6];
        satelliteLink[7] <--> packetGenerator.satelliteLink[7];
        satelliteLink[8] <--> packetGenerator.satelliteLink[8];
        satelliteLink[9] <--> packetGenerator.satelliteLink[9];
        satelliteLink[10] <--> packetGenerator.satelliteLink[10];
        satelliteLink[11] <--> packetGenerator.satelliteLink[11];
        satelliteLink[12] <--> packetGenerator.satelliteLink[12];
        satelliteLink[13] <--> packetGenerator.satelliteLink[13];
        satelliteLink[14] <--> packetGenerator.satelliteLink[14];
        satelliteLink[15] <--> packetGenerator.satelliteLink[15];
        satelliteLink[16] <--> packetGenerator.satelliteLink[16];
        satelliteLink[17] <--> packetGenerator.satelliteLink[17];
        satelliteLink[18] <--> packetGenerator.satelliteLink[18];
        satelliteLink[19] <--> packetGenerator.satelliteLink[19];
        satelliteLink[20] <--> packetGenerator.satelliteLink[20];
        satelliteLink[21] <--> packetGenerator.satelliteLink[21];
        satelliteLink[22] <--> packetGenerator.satelliteLink[22];
        satelliteLink[23] <--> packetGenerator.satelliteLink[23];
        satelliteLink[24] <--> packetGenerator.satelliteLink[24];
        satelliteLink[25] <--> packetGenerator.satelliteLink[25];
        satelliteLink[26] <--> packetGenerator.satelliteLink[26];
        satelliteLink[27] <--> packetGenerator.satelliteLink[27];
        satelliteLink[28] <--> packetGenerator.satelliteLink[28];
        satelliteLink[29] <--> packetGenerator.satelliteLink[29];
        satelliteLink[30] <--> packetGenerator.satelliteLink[30];
        satelliteLink[31] <--> packetGenerator.satelliteLink[31];
        satelliteLink[32] <--> packetGenerator.satelliteLink[32];
        satelliteLink[33] <--> packetGenerator.satelliteLink[33];
        satelliteLink[34] <--> packetGenerator.satelliteLink[34];
        satelliteLink[35] <--> packetGenerator.satelliteLink[35];
        satelliteLink[36] <--> packetGenerator.satelliteLink[36];
        satelliteLink[37] <--> packetGenerator.satelliteLink[37];
        satelliteLink[38] <--> packetGenerator.satelliteLink[38];
        satelliteLink[39] <--> packetGenerator.satelliteLink[39];
}