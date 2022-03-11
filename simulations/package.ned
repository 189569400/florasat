package flora.simulations;

import inet.applications.udpapp.UdpBasicApp;
import inet.node.inet.Router;
import inet.node.internetcloud.InternetCloud;
import inet.node.inet.StandardHost;
import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.node.ethernet.Eth1G;
import inet.visualizer.common.IntegratedVisualizer;
import flora.LoRaPhy.LoRaMedium;
import flora.LoraNode.LoRaNode;
import flora.LoraNode.LoRaGW;
import flora.LoRa.ISLChannel;

@license(LGPL);
network FLoRaSatNetwork
{
    parameters:
        int numberOfNodes = default(1);
        int numberOfGateways = default(1);
        int networkSizeX = default(70000);
        int networkSizeY = default(70000);
        int networkSizeZ = default(70000);
        double del @unit(s) = default(0s);
        double dis @unit(m) = default(40000m);
        int ISLDatarate @unit(kbps)= default(20kbps);

        @display("bgb=28260,19980;bgi=maps/earth,s;bgg=5000,2,grey95;bgu=km");

    types:
        channel DatarateChannel2 extends ned.DatarateChannel
        {
            delay = dis / 200000km * 1s;
            datarate = 20kbps;
            @display("ls=blue,2");
        }

    submodules:
        loRaNodes[numberOfNodes]: LoRaNode {
            @display("p=5555.552,9832.128;is=vs");
        }
        loRaGW[numberOfGateways]: LoRaGW {
            @display("p=3157.472,6394.88;is=s");
        }
        LoRaMedium: LoRaMedium {
            @display("p=8673.056,1159.072");
        }
        networkServer: StandardHost {
                @display("p=14228.607,8633.088;is=s");
        }
        configurator: Ipv4NetworkConfigurator {
            parameters:
                assignDisjunctSubnetAddresses = false;
                @display("p=4836.128,1199.04");
        }
        internetCloud: InternetCloud {
            @display("p=14228.607,5395.6797;is=s");
        }
        gwRouter: Router {
            @display("p=14228.607,3756.992;is=s");
        }
        nsRouter: Router {
            @display("p=14228.607,6914.464;is=s");
        }
        visualizer: IntegratedVisualizer {
            @display("p=1159.072,1199.04");
        }
    connections allowunconnected:

		// Network Server, Ground Station, Satellite 15
        networkServer.ethg++ <--> Eth1G <--> nsRouter.ethg++;
        nsRouter.pppg++ <--> Eth1G <--> internetCloud.pppg++;
        internetCloud.pppg++ <--> {  datarate = 100Mbps; delay = 0.002s; } <--> gwRouter.pppg++;
        gwRouter.ethg++ <--> {  datarate = 100Mbps; delay = 0.002s; } <--> loRaGW[15].ethg;

		// Inter-Satellite Links (grid shape)
        loRaGW[0].up <--> {  datarate = parent.ISLDatarate; delay = 0s;@display("ls=blue,2"); } <--> loRaGW[1].down;
        loRaGW[0].right <--> {  datarate = parent.ISLDatarate; delay = 0s;@display("ls=blue,2"); } <--> loRaGW[4].left;
        loRaGW[1].up <--> {  datarate = parent.ISLDatarate; delay = 0s;@display("ls=blue,2"); } <--> loRaGW[2].down;
        loRaGW[1].right <--> {  datarate = parent.ISLDatarate; delay = 0s;@display("ls=blue,2"); } <--> loRaGW[5].left;
        loRaGW[2].up <--> {  datarate = parent.ISLDatarate; delay = 0s;@display("ls=blue,2"); } <--> loRaGW[3].down;
        loRaGW[2].right <--> {  datarate = parent.ISLDatarate; delay = 0s;@display("ls=blue,2"); } <--> loRaGW[6].left;
        loRaGW[4].up <--> {  datarate = parent.ISLDatarate; delay = 0s;@display("ls=blue,2"); } <--> loRaGW[5].down;
        loRaGW[4].right <--> {  datarate = parent.ISLDatarate; delay = 0s;@display("ls=blue,2"); } <--> loRaGW[8].left;
        loRaGW[5].up <--> {  datarate = parent.ISLDatarate; delay = 0s;@display("ls=blue,2"); } <--> loRaGW[6].down;
        loRaGW[5].right <--> {  datarate = parent.ISLDatarate; delay = 0s;@display("ls=blue,2"); } <--> loRaGW[9].left;
        loRaGW[6].up <--> {  datarate = parent.ISLDatarate; delay = 0s;@display("ls=blue,2"); } <--> loRaGW[7].down;
        loRaGW[6].right <--> {  datarate = parent.ISLDatarate; delay = 0s;@display("ls=blue,2"); } <--> loRaGW[10].left;
        loRaGW[8].up <--> {  datarate = parent.ISLDatarate; delay = 0s;@display("ls=blue,2"); } <--> loRaGW[9].down;
        loRaGW[8].right <--> {  datarate = parent.ISLDatarate; delay = 0s;@display("ls=blue,2"); } <--> loRaGW[12].left;
        loRaGW[9].up <--> {  datarate = parent.ISLDatarate; delay = 0s;@display("ls=blue,2"); } <--> loRaGW[10].down;
        loRaGW[9].right <--> {  datarate = parent.ISLDatarate; delay = 0s;@display("ls=blue,2"); } <--> loRaGW[13].left;
        loRaGW[10].up <--> {  datarate = parent.ISLDatarate; delay = 0s;@display("ls=blue,2"); } <--> loRaGW[11].down;
        loRaGW[10].right <--> {  datarate = parent.ISLDatarate; delay = 0s;@display("ls=blue,2"); } <--> loRaGW[14].left;
        loRaGW[3].right <--> {  datarate = parent.ISLDatarate; delay = 0s;@display("ls=blue,2"); } <--> loRaGW[7].left;
        loRaGW[7].right <--> {  datarate = parent.ISLDatarate; delay = 0s;@display("ls=blue,2"); } <--> loRaGW[11].left;
        loRaGW[11].right <--> {  datarate = parent.ISLDatarate; delay = 0s;@display("ls=blue,2"); } <--> loRaGW[15].left;
        loRaGW[12].up <--> {  datarate = parent.ISLDatarate; delay = 0s;@display("ls=blue,2"); } <--> loRaGW[13].down;
        loRaGW[13].up <--> {  datarate = parent.ISLDatarate; delay = 0s;@display("ls=blue,2"); } <--> loRaGW[14].down;
        loRaGW[14].up <--> {  datarate = parent.ISLDatarate; delay = 0s;@display("ls=blue,2"); } <--> loRaGW[15].down;
}

