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

import inet.applications.contract.IApp;
import inet.queueing.contract.IPacketQueue;

simple ISLPacketForwarder like IApp
{
    @signal[LoRa_GWPacketReceived](type=long); // optional
    @statistic[LoRa_GWPacketReceived](source=LoRa_GWPacketReceived; record=count);
    int test3 = default(1);
    int localPort = default(-1);  // local port (-1: use ephemeral port)
    string destAddresses = default(""); // list of IP addresses, separated by spaces ("": don't send)
    string localAddress = default("");
    int destPort;
    int satelliteID;
    int calculateDistance = default(1);


    gates:
        inout satPart;
        inout left1;
        inout right1;
        inout up1;
        inout down1;
        output socketOut @labels(UdpControlInfo/up);
        input socketIn @labels(UdpControlInfo/down);

        input lowerLayerIn @labels(PacketForwarder/up);
        output lowerLayerOut @labels(PacketForwarder/down);  
    //submodules:
        //queue: <default("DropTailQueue")> like IPacketQueue {
            //parameters:
                //packetCapacity = default(-1);
                //@display("p=100,100;q=l2queue");
        //}
    //connections allowunconnected:

}