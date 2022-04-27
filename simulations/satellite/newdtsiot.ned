package flora.simulations.satellite;

import inet.applications.udpapp.UdpBasicApp;
import inet.node.inet.Router;
import inet.node.internetcloud.InternetCloud;
import inet.node.inet.StandardHost;
import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.node.ethernet.Eth1G;
import inet.visualizer.common.IntegratedVisualizer;
import inet.physicallayer.wireless.unitdisk.UnitDiskRadioMedium;

import flora.LoRaPhy.LoRaMedium;
import flora.LoraNode.LoRaNode;
import flora.LoraNode.LoRaGW;
import flora.ground.GroundStation;
import flora.LoRa.ISLChannel;

import flora.satellite.Satellite;

@license(LGPL);
network NewFLoRaSatNetwork
{
    parameters:
        int nrOfNodes = default(1);
        int nrOfGW = default(1);
        int nrOfGS = default(1);
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

    submodules:
        loRaNodes[nrOfNodes]: LoRaNode {
            @display("p=8667.067,10755.518;is=vs");
        }
        loRaGW[nrOfGW]: Satellite {
            @display("p=8667.067,7205.1523;i=satellit_blue");
        }
        groundStation[nrOfGS]: GroundStation {
            parameters:
                @display("p=9711.292,9015.143;i=device/receiverdish");
        }

        LoRaMedium: LoRaMedium {
            @display("p=1670.76,6021.6973");
        }
        ISLMedium: UnitDiskRadioMedium {
            parameters:
                @display("p=1670.76,8040.532;i=misc/sun");
        }

        networkServer: StandardHost {
            @display("p=14228.607,8633.088;is=s");
        }
        configurator: Ipv4NetworkConfigurator {
            parameters:
                assignDisjunctSubnetAddresses = false;
                @display("p=1670.76,3724.4026");
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
            @display("p=1670.76,1183.455");
        }
        
        
    connections allowunconnected:

		// Network Server in  Ground Station
        networkServer.ethg++ <--> Eth1G <--> nsRouter.ethg++;
        nsRouter.pppg++ <--> Eth1G <--> internetCloud.pppg++;
        internetCloud.pppg++ <--> DatarateChannel2 <--> gwRouter.pppg++;
        gwRouter.ethg++ <--> DatarateChannel2 <--> groundStation[0].ethg;
        
        // GroundStation[0] - Satellite[15] link
        groundStation[0].satelliteLink  <--> DatarateChannel2 <--> loRaGW[nrOfGW-1].groundLink;
}

