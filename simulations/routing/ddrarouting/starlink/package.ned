package flora.simulations.routing.ddrarouting.starlink;

import inet.visualizer.canvas.integrated.IntegratedCanvasVisualizer;
import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;

import flora.LoRaPhy.LoRaMedium;
import flora.LoRa.LoRaNode;
import flora.ground.GroundStationRouting;
import flora.satellite.SatelliteRouting;
import flora.constellation.ConstellationCreator;
import flora.topologycontrol.ConstellationTopologyControl;
import flora.routing.ddra.DDRARouting;
import flora.mobility.SatMobilityOrchestrator;
import flora.networklayer.ConstellationRoutingTable;
import flora.events.EventManager;

@license(LGPL);
network DDRARoutingStarlink
{
    parameters:
        @display("bgb=2160,1080;bgi=background_earth,s");
        int nrOfGS = default(7);
        int nrOfGW = default(300);

    submodules:
        groundStation[nrOfGS]: GroundStationRouting;
        loRaGW[nrOfGW]: SatelliteRouting;

        configurator: Ipv4NetworkConfigurator {
            @display("p=25,273.375;i=block/routing");
        }; // network autoconfiguration

        routing: DDRARouting {
            @display("p=25,373.375;i=block/routing");
        }

        constellationCreator: ConstellationCreator {
            @display("p=25,473.375;i=block/cogwheel");
        }

        topologyControl: ConstellationTopologyControl {
            @display("p=25,573.375;i=block/network2");
        }

        satMobilityOrchestrator: SatMobilityOrchestrator {
            @display("p=25,673.375;i=block/rightarrow");
        }

        constellationRoutingTable: ConstellationRoutingTable {
            @display("p=25,773.375;i=block/network2");
        }

        visualizer: IntegratedCanvasVisualizer {
            @display("p=25,873.375");
        }

        eventManager: EventManager {
            @display("p=25,973.375");
        }

    connections allowunconnected:
        
}
