# FLoRaSat

FLoRaSat (Framework for LoRa-based Satellite networks) is an Omnet++ based discrete-event simulator to carry out end-to-end satellite IoT simulations based on LoRa and LoRaWAN adaptations to the space domain.

The simulator is based on an extended version of [FLoRa](https://flora.aalto.fi/), with satellite trajectories, antenna models, and inter-satellite linking (ISL) fowarding, among other feautures.

Currently, we support a single sample scenario comprising 16 satellites in a grid-like formation (realistic orbital parameters), passing over a circular region with up to 1500 nodes.

Please consider the the simulator is under active development, and it should **not** be considered a final stable version at the moment.


## Installation

1. Install OpenSSL with `sudo apt-get install libssl-dev`

2. Install [OMNeT++6.0.1](https://doc.omnetpp.org/omnetpp/InstallGuide.pdf). Tips:

    * Set the omnetpp environment permanently with `echo '[ -f "$HOME/omnetpp-6.0.1/setenv" ] && source "$HOME/omnetpp-6.0.1/setenv"' >> ~/.profile`

    * Remember to compile with `make -j8` to take advantage of multiple processor cores

    * If **ERROR: /home/diego/omnetpp-6.0.1/bin is not in the path!**, add it by entering `export PATH=$HOME/omnetpp-6.0.1/bin:$PATH`

    * If **ERROR: make: xdg-desktop-menu: No such file or directory** do `sudo apt install xdg-utils`
    
3. Launch omnetpp from the terminal with `omnetpp` and choose a workspace for project (default is `$HOME/omnetpp-6.0.1/samples`)

4. Go to **Help >> Install Simulation Models...** menu and install **INETv4.3.x** in the workspace

5. Clone `https://gitlab.inria.fr/jfraire/florasat.git` in the workspace

6. Add INETv4.3 to the environment by executing florasat/setinet.sh passing the absolute path to the INET root directory, eg. `sh setinet.sh $HOME/omnetpp-6.0.1/samples/inet4.3`

7. In omnetpp go to **File >> Open Projects from File System** and add florasat project to the workspace

8. Right-click florasat project and go to *Properties*, under *Project References* select inet4.3 (only)

9. Finally, right-click florasat and Build Project



## Run simulations

Two scenarios are under development:

- In `/simulations/satelliteradio` the satellites/gateways use radio modules for inter satellite communication. This functionality does not work yet but it is open for development

- In `/simulations/satellitewired` the satellites/gateways use direct links for inter satellite communication




