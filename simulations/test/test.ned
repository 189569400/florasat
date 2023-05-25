package flora.simulations.test;

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
import flora.ground.GroundStation;
import flora.satellite.SatelliteWired;

@license(LGPL);
network TestNetwork
{
    parameters:
        int nrOfNodes = default(1);
        int nrOfGW = default(1);
        int nrOfGS = default(1);
        @display("bgb=2160,1080;bgi=earth_color,s");

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
        loRaNodes[nrOfNodes]: LoRaNode {
            @display("p=695.185,733.055;is=vs");
        }
        loRaGW[nrOfGW]: SatelliteWired {
            @display("p=695.185,549.115;i=satellit_blue");
        }
        groundStation[nrOfGS]: GroundStation {
            @display("p=692.48,340.83;i=device/receiverdish");
        }
        LoRaMedium: LoRaMedium {
            @display("p=124.43,297.55");
        }

        dummyMedium: UnitDiskRadioMedium {
            @display("p=121.725,927.815;is=s");
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
        }
    connections allowunconnected:

		// Network Server in  Ground Station
        networkServer.ethg++ <--> Eth1G <--> nsRouter.ethg++;
        nsRouter.pppg++ <--> Eth1G <--> internetCloud.pppg++;
        internetCloud.pppg++ <--> DatarateChannel2 <--> gwRouter.pppg++;
        gwRouter.ethg++ <--> DatarateChannel2 <--> groundStation[0].ethg;
        groundStation[0].satelliteLink <--> DatarateChannel2 <--> loRaGW[0].groundLink;
        
        //loRaGW[0].up    <--> DatarateChannel1 <--> loRaGW[1].down;
        //loRaGW[0].right <--> DatarateChannel1 <--> loRaGW[2].left;
        //loRaGW[1].right <--> DatarateChannel1 <--> loRaGW[3].left;
        //loRaGW[2].up    <--> DatarateChannel1 <--> loRaGW[3].down;

}
