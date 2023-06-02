import inet.common.TagBase;

namespace inet;

class GroundLinkTag extends TagBase
{
    int gateIndex;
    int dstGs;
    int firstSat;
    int lastSat;
}