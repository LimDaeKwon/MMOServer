
#pragma once

struct SectorPos
{
    unsigned int X;
    unsigned int Y;
};

struct SectorAround
{
    unsigned int Count;
    SectorPos Around[9];
};