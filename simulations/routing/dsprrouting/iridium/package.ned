package flora.simulations.routing.dsprrouting.iridium;

import inet.node.inet.Router;
import inet.node.inet.StandardHost;
import inet.node.inet.INetworkNode;
import inet.node.ethernet.Eth1G;
import inet.node.internetcloud.InternetCloud;
import inet.node.ethernet.Eth10G;
import inet.node.ethernet.Eth100M;
import inet.node.ethernet.EthernetSwitch;
import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.visualizer.common.IntegratedVisualizer;
import inet.visualizer.canvas.integrated.IntegratedCanvasVisualizer;
import inet.physicallayer.wireless.unitdisk.UnitDiskRadioMedium;
import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;

import flora.LoRaPhy.LoRaMedium;
import flora.LoRa.LoRaNode;
import flora.ground.GroundStationRouting;
import flora.satellite.SatelliteRouting;
import flora.constellation.ConstellationCreator;
import flora.topologycontrol.ConstellationTopologyControl;
import flora.routing.dspr.DsprRouting;
import flora.mobility.SatMobilityOrchestrator;
import flora.networklayer.ConstellationRoutingTable;
import flora.events.EventManager;


@license(LGPL);
network DsprRoutingIridium
{
    parameters:
        @display("bgb=2160,1080;bgi=background_earth,s");
        int nrOfGS = default(7);
        int nrOfGW = default(66);

    submodules:
        groundStation[nrOfGS]: GroundStationRouting;
        loRaGW[nrOfGW]: SatelliteRouting;

        configurator: Ipv4NetworkConfigurator {
            @display("p=25,273.375;i=block/routing");
        }; // network autoconfiguration

        routing: DsprRouting {
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
