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
        @display("bgb=2160,1080;bgi=background_earth,s");

    types:
        channel DatarateChannel2 extends ned.DatarateChannel
        {
            datarate = 100Mbps;
            delay = 0.002s;
            @display("ls=black,1");
        }

    submodules:
        loRaNodes[nrOfNodes]: LoRaNode {
            @display("p=695.185,733.055;is=vs");
        }
        loRaGW[nrOfGW]: Satellite {
            @display("p=784.45,624.855;i=satellit_blue");
        }
        groundStation[nrOfGS]: GroundStation {
            @display("p=692.48,481.49;i=device/receiverdish");
        }
        LoRaMedium: LoRaMedium {
            @display("p=124.43,297.55");
        }
        ISLMedium: UnitDiskRadioMedium {
            @display("p=124.43,481.49;i=misc/sun");
        }
        networkServer: StandardHost {
            @display("p=1117.165,733.055;is=s");
        }
        configurator: Ipv4NetworkConfigurator {
            parameters:
                assignDisjunctSubnetAddresses = false;
                @display("p=124.43,102.79");
        }
        internetCloud: InternetCloud {
            @display("p=1117.165,462.555;is=s");
        }
        gwRouter: Router {
            @display("p=1117.165,343.535;is=s");
        }
        nsRouter: Router {
            @display("p=1117.165,586.985;is=s");
        }
        visualizer: IntegratedVisualizer {
        	@display("p=124.43,675.0");
        };
    connections allowunconnected:

		// Network Server in  Ground Station
        networkServer.ethg++ <--> Eth1G <--> nsRouter.ethg++;
        nsRouter.pppg++ <--> Eth1G <--> internetCloud.pppg++;
        internetCloud.pppg++ <--> DatarateChannel2 <--> gwRouter.pppg++;
        gwRouter.ethg++ <--> DatarateChannel2 <--> groundStation[0].ethg;

        // GroundStation[0] - Satellite[15] link
        groundStation[0].satelliteLink <--> DatarateChannel2 <--> loRaGW[nrOfGW-1].groundLink;
}

