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

package flora.LoRa;


//simple ISLChannel
//{
     //gates:
    //    input in;
  //      output out;
//}

channel ISLChannel extends ned.DatarateChannel
{
    //int test9 = default(6);
    double distance @unit(m) = default(40000m);
    //int delay2 = LoRaNode.gatewayY;this.distance
    delay = distance / 200000km * 1s;
    //delay = 100us;
    datarate=100Mbps;
    @display("ls=blue,2");
}