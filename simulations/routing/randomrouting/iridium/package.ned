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
import flora.mobility.FasterSatMobility;
import flora.metrics.MetricsCollector;

@license(LGPL);
network RandomRouteIridium
{
    parameters:
        @display("bgb=2160,1080;bgi=background_earth,s");
        int nrOfGS = default(3);
        int nrOfGW = default(66);

    submodules:
        groundStation[nrOfGS]: GroundStationRouting;

        loRaGW[nrOfGW]: SatelliteRouting;

        routing: RandomRouting {
            @display("p=128,373.375;i=block/routing");
        }

        constellationCreator: ConstellationCreator {
            @display("p=128,473.375;i=block/cogwheel");
        }

        topologyControl: TopologyControl {
            @display("p=128,573.375;i=block/network2");
        }

        fasterSatMobility: FasterSatMobility {
            @display("p=128,673.375;i=block/rightarrow");
        }

        metricsCollector: MetricsCollector {
            @display("p=128,773.375;i=block/blackboard");
        }

        configurator: Ipv4NetworkConfigurator {
            @display("p=128,873.375");
        }

        // <-----><-----><-----><-----><-----><-----><-----><-----><----->
        // <-----><-----><-----><----->NETWORK 0<-----><-----><-----><----->
        // <-----><-----><-----><-----><-----><-----><-----><-----><----->
        router0: Router {
            @display("p=1993.585,373.29");
        }
        switch0: EthernetSwitch {
            @display("p=1993.585,470.66998");
        }
        host0: StandardHost {
            @display("p=2074.735,522.065");
        }
        host1: StandardHost {
            @display("p=1912.4349,522.065");
        }
        host2: StandardHost {
            @display("p=2099.08,340.83");
        }
        // <-----><-----><-----><-----><-----><-----><-----><-----><----->
        // <-----><-----><-----><----->NETWORK 1<-----><-----><-----><----->
        // <-----><-----><-----><-----><-----><-----><-----><-----><----->
        router1: Router {
            @display("p=1071.18,151.48");
        }
        switch1: EthernetSwitch {
            @display("p=1176.675,110.905");
        }
        host3: StandardHost {
            @display("p=1355.205,113.61");
        }
        host4: StandardHost {
            @display("p=1249.71,8.115");
        }
        host5: StandardHost {
            @display("p=1030.605,56.805");
        }
        // <-----><-----><-----><-----><-----><-----><-----><-----><----->
        // <-----><-----><-----><----->NETWORK 2<-----><-----><-----><----->
        // <-----><-----><-----><-----><-----><-----><-----><-----><----->
        router2: Router {
            @display("p=495.01498,289.435");
        }
        switch2: EthernetSwitch {
            @display("p=351.65,259.68");
        }
        host6: StandardHost {
            @display("p=351.65,435.505");
        }
        host7: StandardHost {
            @display("p=289.435,300.255");
        }
        host8: StandardHost {
            @display("p=432.8,416.57");
        }
        // <-----><-----><-----><-----><-----><-----><-----><-----><----->

        visualizer: IntegratedVisualizer {
            @display("p=126.926926,980.04236");
        }

    connections allowunconnected:
        // <-----><-----><-----><-----><-----><-----><-----><-----><----->
        // <-----><-----><----->NETWORK 1<-----><-----><-----><----->
        // <-----><-----><-----><-----><-----><-----><-----><-----><----->
        host0.ethg++ <--> Eth100M <--> switch0.ethg++;
        host1.ethg++ <--> Eth100M <--> switch0.ethg++;
        host2.ethg++ <--> Eth100M <--> switch0.ethg++;
        switch0.ethg++ <--> Eth100M <--> router0.ethg++;

        // <-----><-----><-----><-----><-----><-----><-----><-----><----->
        // <-----><-----><----->NETWORK 2<-----><-----><-----><----->
        // <-----><-----><-----><-----><-----><-----><-----><-----><----->
        host3.ethg++ <--> Eth100M <--> switch1.ethg++;
        host4.ethg++ <--> Eth100M <--> switch1.ethg++;
        host5.ethg++ <--> Eth100M <--> switch1.ethg++;
        switch1.ethg++ <--> Eth100M <--> router1.ethg++;

        // <-----><-----><-----><-----><-----><-----><-----><-----><----->
        // <-----><-----><----->NETWORK 3<-----><-----><-----><----->
        // <-----><-----><-----><-----><-----><-----><-----><-----><----->
        host6.ethg++ <--> Eth100M <--> switch2.ethg++;
        host7.ethg++ <--> Eth100M <--> switch2.ethg++;
        host8.ethg++ <--> Eth100M <--> switch2.ethg++;
        switch2.ethg++ <--> Eth100M <--> router2.ethg++;

        // <-----><-----><-----><-----><-----><-----><-----><-----><----->
        // <-----><-----><----->INTER-NET CONNECTIONS<-----><-----><----->
        // <-----><-----><-----><-----><-----><-----><-----><-----><----->
        router0.ethg++ <--> Eth1G <--> groundStation[0].ethg++;
        router1.ethg++ <--> Eth1G <--> groundStation[1].ethg++;
        router2.ethg++ <--> Eth1G <--> groundStation[2].ethg++;
}
