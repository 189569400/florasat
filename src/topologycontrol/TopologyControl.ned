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

package flora.topologycontrol;

simple TopologyControl
{
    parameters:
        @class(topologycontrol::TopologyControl);
        double updateInterval @unit(s) = default(1s);
        int numGroundLinks = default(40);
        // must be delta or star
        string walkerType;
        double upperLatitudeBound = default(70.0);
        double lowerLatitudeBound = default(-70.0);
        bool isClosedConstellation = default(true);
        
        // speed of light (s/km)
        double islDelay = default(0.0000033356);
        // 2.5 GBit/s
        double islDatarate = default(2500000000);
        // speed of light (s/km)
        double groundlinkDelay = default(0.0000033356);

        // 10 MBit/s
        double groundlinkDatarate = default(10000000);
        double minimumElevation = default(10.0);
}