#pragma once

#include "Sector.h"

struct Session;

struct Character
{
    unsigned int sessionId_;
    unsigned int action_;
    Session* characterSession_;
    unsigned char direction_;

    short x_;
    short y_;
    unsigned char hp_;
    bool isMove_;

    SectorPos oldSectorPos_;
    SectorPos characterSectorPos_;
};
//
//unsigned int sessionId_;
//unsigned int action_;
//Session* characterSession_;
//unsigned char direction_;
//
//short x_;
//short y_;
//unsigned char hp_;
//bool isMove_;
//
//SectorPos oldSectorPos_;
//SectorPos characterSectorPos_;