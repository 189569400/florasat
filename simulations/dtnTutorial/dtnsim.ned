package flora.simulations.dtnTutorial;

import flora.dtn.node.Node;
import flora.dtn.central.Central;
import flora.LoRaPhy.LoRaMedium;
import flora.LoRa.LoRaNode;
import flora.ground.GroundStationRouting;
import flora.satellite.SatelliteRouting;
import flora.constellation.ConstellationCreator;
import flora.topologycontrol.DtnTopologyControl;
import flora.routing.random.RandomRouting;
import flora.routing.directed.DirectedRouting;
import flora.mobility.SatMobilityOrchestrator;
import flora.metrics.MetricsCollector;
import flora.routing.dtn.contactplan.ContactPlan;
import inet.visualizer.common.IntegratedVisualizer;

network DtnSim
{
    parameters:
        int nrOfGS = default(2);
        int nrOfGW = default(3);
        @display("bgb=2160,1080;bgi=background_earth,s");

    submodules:
        groundStation[nrOfGS]: GroundStationRouting {
            @display("p=692.48,340.83;i=device/receiverdish");
        }

        loRaGW[nrOfGW]: SatelliteRouting {
            @display("p=695.185,549.115;i=satellit_blue");
        }

        routing: DirectedRouting {
            @display("p=-128,373.375");
        }
        
        contactPlan: ContactPlan {
        	@display("p=-128,373.375");
        }

        constellationCreator: ConstellationCreator {
            @display("p=-128,473.375");
        }

        topologyControl: DtnTopologyControl {
            @display("p=-128,573.375");
        }

        fasterSatMobility: SatMobilityOrchestrator {
            @display("p=-128,673.375");
        }

        metricsCollector: MetricsCollector {
            @display("p=-128,773.375");
        }

        visualizer: IntegratedVisualizer {
            @display("p=-128,875.0");
        }

    connections allowunconnected:
}
