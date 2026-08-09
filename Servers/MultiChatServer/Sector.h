#pragma once

struct SectorPosition
{
    unsigned int x_;
    unsigned int y_;
};

struct SectorAround
{
    unsigned int count_;
    SectorPosition around_[9];
};