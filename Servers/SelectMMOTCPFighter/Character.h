#pragma once

#include "Sector.h"

struct SESSION;

struct CHARACTER
{
    unsigned int SessionID;
    unsigned int Action;
    SESSION* CharacterSession;
    unsigned char Direction;

    short X;
    short Y;
    unsigned char HP;
    bool IsMove;

    SectorPos OldSectorPos;
    SectorPos CharacterSectorPos;
};