# FLoRaSat

FLoRaSat (Framework for LoRa-based Satellite networks) is an Omnet++ based discrete-event simulator to carry out end-to-end satellite IoT simulations based on LoRa and LoRaWAN adaptations to the space domain.

The simulator is based on an extended version of [FLoRa](https://flora.aalto.fi/), with satellite trajectories, antenna models, and inter-satellite linking (ISL) fowarding, among other feautures.

Currently, we support a single sample scenario comprising 16 satellites in a grid-like formation (realistic orbital parameters), passing over a circular region with up to 1500 nodes.

Please consider the the simulator is under active development, and it should **not** be considered a final stable version at the moment.


## Installation

- The simulator uses OpenSSL headers and libraries. Install with `sudo apt-get install libssl-dev` in Ubuntu

- Install [Omnetpp6.0Pre14](https://omnetpp.org/download/preview) (but should work on later Pre releases)

- Install [INETv4.3](https://inet.omnetpp.org/Installation.html)

- Add INETv4.3 to the environment by executing setinet.sh passing the absolute path to the INET root directory. Example: `sh setinet.sh /home/diego/Diego/AGORA/omnetprojects/inet4.3`

- Copy FLoRaSat resources to the Omnetpp6 reosources. Execute `sh setup.sh`

**IMPORTANT** FLoRaSat will not with Omnetpp5 due to source code path inconsistency with version Omnetpp6


## Run simulations

- The scenario is executed using the `omnetpp.ini` located in the `/simulations` directory




