import inet.common.TagBase;

namespace flora::routing;

enum Dir {
    LEFT = 0;
    UP = 1;
    RIGHT = 2;
    DOWN = 3;
    GROUNDLINK = 4;
};

class NextGateTag extends inet::TagBase
{
    Dir dir;
    bool reportDrop;
}