package flora.simulations.dtnTutorial;

import flora.dtn.node.Node;
import flora.dtn.central.Central;
import flora.LoRaPhy.LoRaMedium;
import flora.LoRa.LoRaNode;
import flora.ground.GroundStationRouting;
import flora.satellite.SatelliteRouting;
import flora.constellation.ConstellationCreator;
import flora.topologycontrol.DtnTopologyControl;
import flora.routing.random.RandomRouting;
import flora.routing.dspr.DsprRouting;
import flora.mobility.SatMobilityOrchestrator;
import flora.routing.dtn.contactplan.ContactPlan;
import inet.visualizer.common.IntegratedVisualizer;
import flora.networklayer.ConstellationRoutingTable;


network DtnSim
{
    parameters:
        int nrOfGS = default(2);
        int nrOfGW = default(8);
        @display("bgb=2160,1080;bgi=background_earth,s");

    submodules:
        groundStation[nrOfGS]: GroundStationRouting;
        loRaGW[nrOfGW]: SatelliteRouting;
        routing: DsprRouting {
            @display("p=-128,373.375;i=block/routing");
        }
        
        contactPlan: ContactPlan {
        	@display("p=-128,373.375");
        }

        constellationCreator: ConstellationCreator {
            @display("p=-128,473.375");
        }

        topologyControl: DtnTopologyControl {
            @display("p=-128,573.375");
        }

        satMobilityOrchestrator: SatMobilityOrchestrator {
            @display("p=-128,673.375;i=block/rightarrow");
        }
        
        constellationRoutingTable: ConstellationRoutingTable {
            @display("p=-128,773.375;i=block/network2");
        }
        

        visualizer: IntegratedVisualizer {
            @display("p=-128,875.0");
        }

    connections allowunconnected:
}
