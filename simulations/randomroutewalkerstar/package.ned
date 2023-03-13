package flora.simulations.randomroutewalkerstar;

import inet.applications.udpapp.UdpBasicApp;
import inet.node.inet.Router;
import inet.node.internetcloud.InternetCloud;
import inet.node.inet.StandardHost;
import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.node.ethernet.Eth1G;
import inet.visualizer.common.IntegratedVisualizer;
import inet.physicallayer.wireless.unitdisk.UnitDiskRadioMedium;

import flora.LoRaPhy.LoRaMedium;
import flora.LoRa.LoRaNode;
import flora.ground.GroundStationRouting;
import flora.satellite.SatelliteRouting;
import flora.constellation.ConstellationCreator;
import flora.topologycontrol.TopologyControl;
import flora.routing.random.RandomRouting;
import flora.routing.directed.DirectedRouting;
import flora.mobility.FasterSatMobility;
import flora.metrics.MetricsCollector;

@license(LGPL);
network RandomRoute
{
    parameters:
        int nrOfGS = default(9);
        int nrOfGW = default(66);
        @display("bgb=2160,1080;bgi=background_earth,s");

    submodules:
        groundStation[nrOfGS]: GroundStationRouting {
            @display("p=692.48,340.83;i=device/receiverdish");
        }

        loRaGW[nrOfGW]: SatelliteRouting {
            @display("p=695.185,549.115;i=satellit_blue");
        }

        // loRaNodes[nrOfNodes]: LoRaNode {
        //     @display("p=695.185,733.055;is=vs");
        // }

        routing: DirectedRouting {
            @display("p=-128,373.375");
        }

        constellationCreator: ConstellationCreator {
            @display("p=-128,473.375");
        }

        topologyControl: TopologyControl {
            @display("p=-128,573.375");
        }

        fasterSatMobility: FasterSatMobility {
            @display("p=-128,673.375");
        }

        metricsCollector: MetricsCollector {
            @display("p=-128,773.375");
        }

        // LoRaMedium: LoRaMedium {
        //     @display("p=124.43,297.55");
        // }

        // dummyMedium: UnitDiskRadioMedium {
        //     @display("p=121.725,927.815;is=s");
        // }

        // networkServer: StandardHost {
        //     @display("p=1117.165,733.055;is=s");
        // }
        
        // configurator: Ipv4NetworkConfigurator {
        //     parameters:
        //         assignDisjunctSubnetAddresses = false;
        //         @display("p=124.43,102.79");
        // }

        // internetCloud: InternetCloud {
        //     @display("p=1117.165,462.555;is=s");
        // }

        // gwRouter: Router {
        //     @display("p=1117.165,343.535;is=s");
        // }

        // nsRouter: Router {
        //     @display("p=1117.165,586.985;is=s");
        // }

        visualizer: IntegratedVisualizer {
            @display("p=-128,875.0");
        }

    connections allowunconnected:

}