package flora.simulations.classic;

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
import flora.ground.GroundStation;
import flora.LoRa.ISLChannel;

@license(LGPL);
network FLoRaSatNetwork
{
    parameters:
        int nrOfND = default(1);
        int nrOfGW = default(1);
        int networkSizeX = default(70000);
        int networkSizeY = default(70000);
        int networkSizeZ = default(70000);

        @display("bgb=27842.156,15554.88;bgi=background_earth,s");

    types:
        channel DatarateChannel2 extends ned.DatarateChannel
        {
            datarate = 100Mbps;
            delay = 0.002s;
            @display("ls=black,1");
        }
        
        channel DatarateChannel1 extends ned.DatarateChannel
        {
            datarate = 20kbps;
            delay = 0s;
            @display("ls=red,2");
        }

    submodules:
        loRaNodes[nrOfND]: LoRaNode {
            @display("p=4385.745,9363.218;is=vs");
        }
        loRaGW[nrOfGW]: LoRaGW {
            @display("p=3157.472,6394.88;i=satellit_blue");
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
        nsRouter.pppg++      <--> Eth1G <--> internetCloud.pppg++;
        internetCloud.pppg++ <--> DatarateChannel2 <--> gwRouter.pppg++;
        gwRouter.ethg++      <--> DatarateChannel2 <--> loRaGW[15].ethg;

		// Inter-Satellite Links (grid shape)
        loRaGW[0].up    <--> DatarateChannel1 <--> loRaGW[1].down;
        loRaGW[0].right <--> DatarateChannel1 <--> loRaGW[4].left;
        loRaGW[1].up    <--> DatarateChannel1 <--> loRaGW[2].down;
        loRaGW[1].right <--> DatarateChannel1 <--> loRaGW[5].left;
        loRaGW[2].up    <--> DatarateChannel1 <--> loRaGW[3].down;
        loRaGW[2].right <--> DatarateChannel1 <--> loRaGW[6].left;
        loRaGW[3].right <--> DatarateChannel1 <--> loRaGW[7].left;

        loRaGW[4].up    <--> DatarateChannel1 <--> loRaGW[5].down;
        loRaGW[4].right <--> DatarateChannel1 <--> loRaGW[8].left;
        loRaGW[5].up    <--> DatarateChannel1 <--> loRaGW[6].down;
        loRaGW[5].right <--> DatarateChannel1 <--> loRaGW[9].left;
        loRaGW[6].up    <--> DatarateChannel1 <--> loRaGW[7].down;
        loRaGW[6].right <--> DatarateChannel1 <--> loRaGW[10].left;
        loRaGW[7].right <--> DatarateChannel1 <--> loRaGW[11].left;

        loRaGW[8].up     <--> DatarateChannel1 <--> loRaGW[9].down;
        loRaGW[8].right  <--> DatarateChannel1 <--> loRaGW[12].left;
        loRaGW[9].up     <--> DatarateChannel1 <--> loRaGW[10].down;
        loRaGW[9].right  <--> DatarateChannel1 <--> loRaGW[13].left;
        loRaGW[10].up    <--> DatarateChannel1 <--> loRaGW[11].down;
        loRaGW[10].right <--> DatarateChannel1 <--> loRaGW[14].left;
        loRaGW[11].right <--> DatarateChannel1 <--> loRaGW[15].left;

        loRaGW[12].up <--> DatarateChannel1 <--> loRaGW[13].down;
        loRaGW[13].up <--> DatarateChannel1 <--> loRaGW[14].down;
        loRaGW[14].up <--> DatarateChannel1 <--> loRaGW[15].down;
        
}

