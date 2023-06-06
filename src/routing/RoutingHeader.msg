import inet.common.INETDefs;
import inet.common.Units;
import inet.common.packet.chunk.Chunk;

cplusplus {{
using namespace inet;
}}

namespace flora;

enum ToS {
    LOW = 0;
    NORMAL = 1;
    HIGH = 2;
};

enum Type {
    G2G = 0;
    S2S = 1;
    SBroadcast = 2;
};

class RoutingHeader extends inet::FieldsChunk {
    ToS tos;
    Type type;
    inet::B totalLength;
    int ident;
    int TTL;
    int srcSat;
    int dstSat;
    int srcGs;
    int dstGs;
}