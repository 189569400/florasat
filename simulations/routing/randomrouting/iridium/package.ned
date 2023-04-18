package flora.simulations.routing.randomrouting.iridium;

import inet.node.inet.Router;
import inet.node.inet.StandardHost;
import inet.node.ethernet.Eth1G;
import inet.node.ethernet.Eth100M;
import inet.node.ethernet.EthernetSwitch;
import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.visualizer.common.IntegratedVisualizer;

import flora.LoRaPhy.LoRaMedium;
import flora.LoRa.LoRaNode;
import flora.ground.GroundStationRouting;
import flora.satellite.SatelliteRouting;
import flora.constellation.ConstellationCreator;
import flora.topologycontrol.TopologyControl;
import flora.routing.random.RandomRouting;
import flora.mobility.SatMobilityOrchestrator;
import flora.metrics.MetricsCollector;

@license(LGPL);
network RandomRouteIridium
{
    parameters:
        @display("bgb=2160,1080;bgi=background_earth,s");
        int nrOfGS = default(7);
        int nrOfGW = default(66);

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

        visualizer: IntegratedVisualizer {
            @display("p=-128,873.375");
        }

    connections allowunconnected:
}
