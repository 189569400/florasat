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

import inet.queueing.base.PacketSinkBase;
import inet.queueing.contract.IActivePacketSink;

simple PacketHandlerRouting extends PacketSinkBase like IActivePacketSink
{
    parameters:
        displayStringTextFormat = default("processed %p pk (%l)\ndropped %d pk");
        @class(satellite::PacketHandlerRouting);
        @display("i=block/app");
        int maxHops = default(50);

        @signal[packetSent](type=inet::Packet);
        @signal[packetReceived](type=inet::Packet);

        volatile double processingDelay @unit(s);

    gates:        
        // ISL gates
        inout left1;
        inout right1;
        inout up1;
        inout down1;
        
        input in @labels(pull);
        output out;

        // GroundStation gates
        inout groundLink1[40] @labels(GroundStationGate);
}