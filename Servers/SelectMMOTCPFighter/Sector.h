
#pragma once

struct SectorPos
{
    unsigned int x_;
    unsigned int y_;
};

struct SectorAround
{
    unsigned int count_;
    SectorPos around_[9];
};

struct SectorUpdateAround
{
    SectorAround removeSector_;
    SectorAround addSector_;
};