# FLoRaSat

FLoRaSat (Framework for LoRa-based Satellite networks) is an Omnet++ based discrete-event simulator to carry out end-to-end satellite IoT simulations based on LoRa and LoRaWAN adaptations to the space domain.

The simulator is based on an extended version of [FLoRa](https://flora.aalto.fi/), with satellite trajectories, antenna models, and inter-satellite linking (ISL) fowarding, among other feautures.

Currently, we support a single sample scenario comprising 16 satellites in a grid-like formation (realistic orbital parameters), passing over a circular region with up to 1500 nodes.

Please consider the the simulator is under active development, and it should **not** be considered a final stable version at the moment.


## Installation

- Install [OMNeT++6.0](https://omnetpp.org/)

- Install [INETv4.3](https://inet.omnetpp.org/Installation.html)

- Install OpenSSL, in Ubuntu use `sudo apt-get install libssl-dev`

- Add INETv4.3 to the environment by executing setinet.sh passing the absolute path to the INET root directory, eg. `sh setinet.sh $HOME/omnetprojects/inet4.3`

- Clone `https://github.com/diegomm6/os3` repo where INET is located and checkout to `framework` branch

- Edit the Makefile in the os3 repo root directory, in line 13 add the correct path for the curl library (eg. `/usr/include/x86_64-linux-gnu/curl`)

- Clone `https://github.com/diegomm6/leosatellites` repo where INET is located and checkout to `framework` branch

- Edit the Makefile in the leosatellites repo root directory, in line 13 add the correct path to the curl library and the path to the os3 src directory

- Clone `https://gitlab.inria.fr/jfraire/florasat.git` repo where INET is located

- Launch OMNeT++ IDE from the terminal with `omnetpp`. Add inet4.3, os3, leosatellites and florasat projects to the workspace

- Go to os3 project Properties, under OMNeT++/Makemake (panel to the left) select Build/Makemake/Options... for src folder (panel to the right), then in Compile include the path to the curl library (eg. `/usr/include/x86_64-linux-gnu/curl`). Also in os3 Properties, under Project References select inet4.3

- Go to leosatellites project Properties, under OMNeT++/Makemake (panel to the left) select Build/Makemake/Options... for src folder (panel to the right), then in Compile include the path to the os3 library (eg. `$HOME/omnetprojects/os3/src`). Also in leosatellites Properties, under Project References select inet4.3 and os3

- Go to florasat project Properties, under OMNET++/Makemake (panel to the left) select Build/Makemake/Options... for src folder (panel to the right), then in Compile include the path to the os3 and leosatellites libraries. Also in florasat Properties, under Project References select inet4.3, os3 and leosatellite

- Finally, Build Project in order: os3, leosatellites, florasat


**IMPORTANT** FLoRaSat will not work with OMNeT++5 due to inconsistency with version OMNeT++6


## Run simulations

Two scenarios are under development:

- In `/simulations/satelliteradio` the satellites/gateways use radio modules for inter satellite communication. This functionality does not work yet but it is open for development

- In `/simulations/satellitewired` the satellites/gateways use direct links for inter satellite communication





