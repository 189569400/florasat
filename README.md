# FLoRaSat

FLoRaSat (Framework for LoRa-based Satellite networks) is an Omnet++ based discrete-event simulator to carry out end-to-end satellite IoT simulations based on LoRa and LoRaWAN adaptations to the space domain.

The simulator is based on an extended version of [FLoRa](https://flora.aalto.fi/), with satellite trajectories, antenna models, and inter-satellite linking (ISL) fowarding, among other feautures.

Currently, we support a single sample scenario comprising 16 satellites in a grid-like formation (realistic orbital parameters), passing over a circular region with up to 1500 nodes.

Please consider the the simulator is under active development, and it should **not** be considered a final stable version at the moment.

## Installation

FLoRaSat was recently tested using: 
- [Omnetpp6.0Pre14](https://omnetpp.org/download/preview) (but should work on later Pre releases)
- INET v4.3 (installed by default the first time Omnetpp IDE is launched)

 The simulator also uses OpenSSL headers and libraries, which should be installed beforehand (`sudo apt-get install libssl-dev` in Ubuntu).
 
 **Note 1:** Move `earth.jpg` from  the simulations directory to `omnetpp-6.0pre14/images/maps` directory.

 **Note 2:** The scenario is executed using the `omnetpp.ini` located in the `/simulations` directory.
