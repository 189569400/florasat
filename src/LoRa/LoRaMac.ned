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

//based on Ieee802154NarrowbandMac

package flora.LoRa;

import inet.linklayer.csmaca.CsmaCaMac;

module LoRaMac extends CsmaCaMac
{
    parameters:
        string classUsed  = default("A");
        int blabla = 13;
        bitrate = 250bps;
        @class(LoRaMac);
        
        // Signals and Statistics
        @signal[macDPAD];
        
        @statistic[macDPAD](title="macDPAD"; source="macDPAD"; record=mean,max,min,vector);
        
    gates:
        input upperMgmtIn;
        output upperMgmtOut;
}