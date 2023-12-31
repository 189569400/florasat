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

import inet.queueing.contract.IPacketQueue;
import inet.linklayer.contract.IWirelessInterface;
import inet.networklayer.common.NetworkInterface;
import inet.physicallayer.wireless.common.contract.packetlevel.IRadio;
import inet.physicallayer.wireless.unitdisk.UnitDiskRadioMedium;
import inet.linklayer.acking.AckingMac;

//
// This module implements a highly abstracted wireless network interface (NIC)
// that uses a trivial MAC protocol. It offers simplicity for scenarios where
// Layer 1 and 2 effects can be completely ignored, for example testing the
// basic functionality of a wireless ad-hoc routing protocol.
//
// The most important parameter this model accepts is the transmission range.
// When a radio transmits a frame, all other radios within transmission range
// will receive the frame correctly, and radios that are out of range will not be
// affected at all.
//
// This module requires an ~UnitDiskRadioMedium instance in the network.
//
module DummyNic extends NetworkInterface like IWirelessInterface
{
    parameters:
        string interfaceTableModule;
        string energySourceModule = default("");
        double bitrate @unit(bps) = default(10bps);
        *.interfaceTableModule = default(absPath(this.interfaceTableModule));
        *.energySourceModule = default(absPath(this.energySourceModule));
        **.bitrate = 100Mbps;
        radio.antenna.mobilityModule = "^.^.^.mobility";
        radio.transmitter.headerLength = 4B;
        radio.radioMediumModule = "^.^.^.dummyMedium";
        radio.transmitter.communicationRange = 0m;
    gates:
        input upperLayerIn;
        output upperLayerOut;
        input radioIn @labels(WirelessSignal);
    submodules:
        mac: AckingMac {
            parameters:
                @display("p=300,100");
        }
        radio: <default("UnitDiskRadio")> like IRadio {
            parameters:
                @display("p=300,200");
        }
    connections:
        upperLayerIn --> { @display("m=n"); } --> mac.upperLayerIn;
        mac.lowerLayerOut --> radio.upperLayerIn;

        radioIn --> { @display("m=s"); } --> radio.radioIn;
        radio.upperLayerOut --> mac.lowerLayerIn;
        mac.upperLayerOut --> { @display("m=n"); } --> upperLayerOut;
}
