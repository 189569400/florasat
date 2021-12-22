package flora.simulations;

import inet.applications.udpapp.UdpBasicApp;
import inet.node.inet.Router;
import inet.node.internetcloud.InternetCloud;
import flora.LoRaPhy.LoRaMedium;
import flora.LoraNode.LoRaNode;
import flora.LoraNode.LoRaGW;
import inet.node.inet.StandardHost;
import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.node.ethernet.Eth1G;
import flora.LoRa.ISLChannel;
import inet.visualizer.common.IntegratedVisualizer;
//import new.DataRateChannel2;

@license(LGPL);
network LoRaNetworkTest
{
    parameters:
        int numberOfNodes = default(1);
        int numberOfGateways = default(1);
        int networkSizeX = default(70000);
        int networkSizeY = default(70000);
        int networkSizeZ = default(70000);
        double del @unit(s) = default(0s); 
        double dis @unit(m) = default(40000m);
        int test7 = default(6); //unused
        //LoRaNode.maxGatewayDistance = default(320.0);
        //LoRaNode.SimpleLoRaApp.
        @display("bgb=28260,19980;bgi=maps/earth,s;bgg=5000,2,grey95;bgu=km");
        //this channel is unused for now
    types:    
    	channel DatarateChannel2 extends ned.DatarateChannel
		{
    		//double distance @unit(m) = default(40000m);
    		//int delay2 = LoRaNode.gatewayY;
    		delay = dis / 200000km * 1s;
    		//delay = 100us;
    		datarate=100Mbps;
    		@display("ls=blue,2");
		}
    submodules:
        
        visualizer: IntegratedVisualizer {
            //module to visualize movement
        }
        loRaNodes[numberOfNodes]: LoRaNode {
            @display("p=83.664,233.856;is=vs");
            //SimpleLoRaApp.timeToNextPacket= 10s;
        }
        loRaGW[numberOfGateways]: LoRaGW {
            //gates:
                //port[2];
            @display("p=83.664,150.192;is=s");
        }
        LoRaMedium: LoRaMedium {
            @display("p=167.328,88.704");
        }
        networkServer: StandardHost {
            parameters:
                @display("p=14490.24002,4995.216002;is=s");
        }
        configurator: Ipv4NetworkConfigurator {
            parameters:
                assignDisjunctSubnetAddresses = false;
                @display("p=14480.64801,4995.704");
        }
        internetCloud: InternetCloud {
            @display("p=14460.664,4995.216002;is=s");
        }
        gwRouter[numberOfGateways]: Router {
            @display("p=14450.192001,4995.216002;is=s");
        }
        nsRouter: Router {
            @display("p=14470.09601,4995.216002;is=s");
        }
    connections allowunconnected:
        
        networkServer.ethg++ <--> Eth1G <--> nsRouter.ethg++;
        nsRouter.pppg++ <--> Eth1G <--> internetCloud.pppg++;
        //internetCloud.pppg++ <--> Eth1G <--> gwRouter[15].pppg++;
        //gwRouter[15].ethg++ <--> Eth1G <--> loRaGW[15].ethg++;
        
        //for i=0..numberOfGateways-1 {
            internetCloud.pppg++ <--> {datarate = 100Mbps; delay = 0.002s;} <--> gwRouter[15].pppg++; //connection between sat and gwRouter
            gwRouter[15].ethg++ <--> {datarate = 100Mbps; delay = 0.002s;} <--> loRaGW[15].ethg++;
        //}
        //for i=0..numberOfGateways-2{
        //loRaGW[0].LoRaGWNic.left <--> loRaGW[0].LoRaGWNic.down;
        //ISL
        loRaGW[0].up <--> {datarate = 20kbps; delay = 0s; @display("ls=blue,2");} <--> loRaGW[1].down;
        loRaGW[0].right <--> {datarate = 20kbps; delay = 0s; @display("ls=blue,2");} <--> loRaGW[4].left;
        loRaGW[1].up <--> {datarate = 20kbps; delay = 0s; @display("ls=blue,2");} <--> loRaGW[2].down;
        loRaGW[1].right <--> {datarate = 20kbps; delay = 0s; @display("ls=blue,2");} <--> loRaGW[5].left;
        loRaGW[2].up <--> {datarate = 20kbps; delay = 0s; @display("ls=blue,2");} <--> loRaGW[3].down;
        loRaGW[2].right <--> {datarate = 20kbps; delay = 0s; @display("ls=blue,2");} <--> loRaGW[6].left;
        loRaGW[4].up <--> {datarate = 20kbps; delay = 0s; @display("ls=blue,2");} <--> loRaGW[5].down;
        loRaGW[4].right <--> {datarate = 20kbps; delay = 0s; @display("ls=blue,2");} <--> loRaGW[8].left;
        loRaGW[5].up <--> {datarate = 20kbps; delay = 0s; @display("ls=blue,2");} <--> loRaGW[6].down;
        loRaGW[5].right <--> {datarate = 20kbps; delay = 0s; @display("ls=blue,2");} <--> loRaGW[9].left;
        loRaGW[6].up <--> {datarate = 20kbps; delay = 0s; @display("ls=blue,2");} <--> loRaGW[7].down;
        loRaGW[6].right <--> {datarate = 20kbps; delay = 0s; @display("ls=blue,2");} <--> loRaGW[10].left;
        loRaGW[8].up <--> {datarate = 20kbps; delay = 0s; @display("ls=blue,2");} <--> loRaGW[9].down;
        loRaGW[8].right <--> {datarate = 20kbps; delay = 0s; @display("ls=blue,2");} <--> loRaGW[12].left;
        loRaGW[9].up <--> {datarate = 20kbps; delay = 0s; @display("ls=blue,2");} <--> loRaGW[10].down;
        loRaGW[9].right <--> {datarate = 20kbps; delay = 0s; @display("ls=blue,2");} <--> loRaGW[13].left;
        loRaGW[10].up <--> {datarate = 20kbps; delay = 0s; @display("ls=blue,2");} <--> loRaGW[11].down;
        loRaGW[10].right <--> {datarate = 20kbps; delay = 0s; @display("ls=blue,2");} <--> loRaGW[14].left;
        
        loRaGW[3].right <--> {datarate = 20kbps; delay = 0s; @display("ls=blue,2");} <--> loRaGW[7].left;
        loRaGW[7].right <--> {datarate = 20kbps; delay = 0s; @display("ls=blue,2");} <--> loRaGW[11].left;
        loRaGW[11].right <--> {datarate = 20kbps; delay = 0s; @display("ls=blue,2");} <--> loRaGW[15].left;

        loRaGW[12].up <--> {datarate = 20kbps; delay = 0s; @display("ls=blue,2");} <--> loRaGW[13].down;
        loRaGW[13].up <--> {datarate = 20kbps; delay = 0s; @display("ls=blue,2");} <--> loRaGW[14].down;
        loRaGW[14].up <--> {datarate = 20kbps; delay = 0s; @display("ls=blue,2");} <--> loRaGW[15].down;
        
        
        //}
}

