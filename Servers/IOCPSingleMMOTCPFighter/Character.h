#pragma once

#include "Sector.h"

using SessionId = unsigned __int64;

struct Character
{
    SessionId sessionId_;
    unsigned int action_;
    unsigned char direction_;

    short x_;
    short y_;
    unsigned char hp_;
    bool isMove_;
    int movingIndex_;

    SectorPos oldSectorPos_;
    SectorPos characterSectorPos_;
};
