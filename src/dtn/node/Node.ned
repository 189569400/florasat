package flora.dtn.node;

import flora.dtn.node.fault.Fault;
import flora.dtn.node.graphics.Graphics;
import flora.dtn.node.app.App;
import flora.dtn.node.dtn.Dtn;
import flora.dtn.node.com.Com;

module Node
{
    parameters:
        @display("i=device/satellite");
        string icon = default("satellite");
       
    submodules:
        app: App {
            parameters:
                @display("p=75,50");
        }
        dtn: Dtn {
            parameters:
                @display("p=75,120");
        }
        com: Com {
            parameters:
                @display("p=75,190");
        }
        graphics: Graphics {
            parameters:
                @display("p=150,50");
        }
        fault: Fault {
            parameters:
                @display("p=150,120");
        }

    connections:
        app.gateToDtn <--> dtn.gateToApp;
        dtn.gateToCom <--> com.gateToDtn;
}
