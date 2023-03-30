# FLoRaSat

FLoRaSat (Framework for LoRa-based Satellite networks) is an Omnet++ based discrete-event simulator to carry out end-to-end satellite IoT simulations based on LoRa and LoRaWAN adaptations to the space domain.

The simulator is based on an extended version of [FLoRa](https://flora.aalto.fi/), with satellite trajectories, antenna models, and inter-satellite linking (ISL) fowarding, among other feautures.

Currently, we support a single sample scenario comprising 16 satellites in a grid-like formation (realistic orbital parameters), passing over a circular region with up to 1500 nodes.

Please consider the the simulator is under active development, and it should **not** be considered a final stable version at the moment.


## Installation

1. Install [OMNeT++6.0.1](https://doc.omnetpp.org/omnetpp/InstallGuide.pdf). Tips:

    * Set the omnetpp environment permanently with `echo '[ -f "$HOME/omnetpp-6.0.1/setenv" ] && source "$HOME/omnetpp-6.0.1/setenv"' >> ~/.profile`

    * Remember to compile with `make -j8` to take advantage of multiple processor cores

    * If **ERROR: /home/diego/omnetpp-6.0.1/bin is not in the path!**, add it by entering `export PATH=$HOME/omnetpp-6.0.1/bin:$PATH`

    * If **ERROR: make: xdg-desktop-menu: No such file or directory** do `sudo apt install xdg-utils`

2. Launch omnetpp from the terminal with `omnetpp` and choose a workspace for project (default is `$HOME/omnetpp-6.0.1/samples`)

3. Go to **Help >> Install Simulation Models...** menu and install **INETv4.3.x** in the workspace

4. Install OpenSSL with `sudo apt-get install libssl-dev`

5. Install curl lirary with `sudo apt-get install libcurl4-openssl-dev`

    * Verify the location of the curl library with `find /usr/include -name 'curl.h'`, usually it should be in `/usr/include/x86_64-linux-gnu/curl`

6. Clone `https://gitlab.inria.fr/jfraire/florasat.git` in the workspace

7. Add INETv4.3 to the environment by executing setinet.sh passing the absolute path to the INET root directory, eg. `sh setinet.sh $HOME/omnetpp-6.0.1/samples/inet4.3`

8. Clone `https://github.com/diegomm6/os3` in the workspace and checkout to `framework` branch

9. Clone `https://github.com/diegomm6/leosatellites` in the workspace and checkout to `framework` branch

10.  In omnetpp go to **File >> Open Projects from File System** and add os3, leosatellites and florasat projects to the workspace

11. Right-click os3 project and go to *Properties*, under *OMNeT++/Makemake* (panel to the left) select *Build/Makemake/Options...* for src folder (panel to the right), then in *Compile* include the path to the curl library (eg. `/usr/include/x86_64-linux-gnu/curl`). Also in os3 *Properties*, under *Project References* select inet4.3

12. Right-click leosatellites project and go to *Properties*, under *OMNeT++/Makemake* (panel to the left) select *Build/Makemake/Options...* for src folder (panel to the right), then in Compile include the path to the os3 library (eg. `$HOME/omnetpp-6.0.1/samples/os3/src`). Also in leosatellites *Properties*, under *Project References* select inet4.3 and os3

13. Right-click florasat project and go to *Properties*, under *OMNET++/Makemake* (panel to the left) select *Build/Makemake/Options...* for src folder (panel to the right), then in Compile include the path to the os3 and leosatellites libraries. Also in florasat *Properties*, under *Project References* select inet4.3, os3 and leosatellite

14. Finally, right-click florasat and Build Project



## Run simulations

Two scenarios are under development:

- In `/simulations/satelliteradio` the satellites/gateways use radio modules for inter satellite communication. This functionality does not work yet but it is open for development

- In `/simulations/satellitewired` the satellites/gateways use direct links for inter satellite communication





