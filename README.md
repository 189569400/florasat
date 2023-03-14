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

- Clone `https://gitlab.inria.fr/jfraire/florasat.git` repo where INET is located and checkout to `dev_repomerge` branch

- Launch OMNeT++ IDE from the terminal with `omnetpp`. Add inet4.3 and florasat projects to the workspace

- Go to florasat project Properties under Project References and select inet4.3

- Finally, Build Project florasat


**IMPORTANT** FLoRaSat will not with OMNeT++5 due to source code path inconsistency with version OMNeT++6


## Run simulations

Multiple scenarios are under development on the framework branch:

- In `/simulations/classic` there is no support for ground station. This scenario is the classic implementation of florasat found in the master branch with the addition of orbital propagation

- In `/simulations/satelliteradio` the satellites/gateways use radio modules for inter satellite communication. This functionality does not work yet but it is open for development

- In `/simulations/satellitewired` the satellites/gateways use direct links for inter satellite communication





