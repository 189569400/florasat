package flora.simulations.routing.randomrouting.starlink;

import inet.visualizer.common.IntegratedVisualizer;

import flora.LoRaPhy.LoRaMedium;
import flora.LoRa.LoRaNode;
import flora.ground.GroundStationRouting;
import flora.satellite.SatelliteRouting;
import flora.constellation.ConstellationCreator;
import flora.topologycontrol.TopologyControl;
import flora.routing.random.RandomRouting;
import flora.mobility.SatMobilityOrchestrator;
import flora.networklayer.ConstellationRoutingTable;

@license(LGPL);
network RandomRouteStarLink
{
    parameters:
        @display("bgb=2160,1080;bgi=background_earth,s");
        int nrOfGS = default(7);
        int nrOfGW = default(1584);

    submodules:
        groundStation[nrOfGS]: GroundStationRouting;
        loRaGW[nrOfGW]: SatelliteRouting;
        routing: RandomRouting {
            @display("p=-128,373.375;i=block/routing");
        }

        constellationCreator: ConstellationCreator {
            @display("p=-128,473.375;i=block/cogwheel");
        }

        topologyControl: TopologyControl {
            @display("p=-128,573.375;i=block/network2");
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
