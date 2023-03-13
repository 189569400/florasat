import inet.common.INETDefs;
import inet.common.Units;
import inet.common.packet.chunk.Chunk;

cplusplus {{
using namespace inet;
}}

namespace flora;

class RoutingFrame extends inet::FieldsChunk {
    int sequenceNumber;
    int numHop = 0;
    int sourceGroundstation;
    int destinationGroundstation;

    // timestamp of message creation
    simtime_t originTime;
    // timestamp of message reception
    simtime_t receptionTime;

    // satellite ids in route + timestamps when packet arrived at satellites
    int route[];
    simtime_t timestamps[];
}