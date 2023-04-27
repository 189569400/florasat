package flora.simulations.dtnTutorial;

import flora.dtn.node.Node;
import flora.dtn.central.Central;

network dtnsim
{
    parameters:
        int nodesNumber = default(0);
        @display("bgb=503,408");

    submodules:
        node[nodesNumber+1]: Node {
            parameters:
                @display("p=169,50");
        }

        central: Central {
            parameters:
                @display("p=40,150");
        }

    connections:
}
