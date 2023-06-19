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

package flora.simulations.dtnTutorial.contactplangenerator;

import inet.visualizer.common.IntegratedVisualizer;

import flora.LoRaPhy.LoRaMedium;
import flora.LoRa.LoRaNode;
import flora.ground.DtnGroundStationRouting;
import flora.satellite.SatelliteRouting;
import flora.constellation.ConstellationCreator;
import flora.topologycontrol.ContactPlanTopologyControl;
import flora.routing.directed.DirectedRouting;
import flora.mobility.SatMobilityOrchestrator;
import flora.networklayer.ConstellationRoutingTable;
import flora.routing.dtn.contactplan.ContactPlan;

@license(LGPL);
network ContactPlanGenerator
{
    parameters:
        @display("bgb=2160,1080;bgi=background_earth,s");
        int nrOfGS = default(2);
        int nrOfGW = default(8);

    submodules:
        groundStation[nrOfGS]: DtnGroundStationRouting;
        loRaGW[nrOfGW]: SatelliteRouting;
        
        routing: DirectedRouting {
            @display("p=-128,373.375;i=block/routing");
        }
        
        contactPlan: ContactPlan {
        	@display("p=-128,373.375");
        }

        constellationCreator: ConstellationCreator {
            @display("p=-128,473.375;i=block/cogwheel");
        }

        topologyControl: ContactPlanTopologyControl {
            @display("p=-128,573.375");
        }

        satMobilityOrchestrator: SatMobilityOrchestrator {
            @display("p=-128,673.375;i=block/rightarrow");
        }

        constellationRoutingTable: ConstellationRoutingTable {
            @display("p=-128,773.375;i=block/network2");
        }

        visualizer: IntegratedVisualizer {
            @display("p=-128,873.375");
        }

    connections allowunconnected:
}