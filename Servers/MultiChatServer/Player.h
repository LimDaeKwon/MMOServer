#pragma once

#include "GameDefine.h"
#include "Sector.h"

struct Player
{
    __int64 sessionId_{ 0 };
    __int64 accountNo_{ 0 };

    SectorPosition sectorPosition_{};

    wchar_t id_[PlayerIdLength];
    wchar_t nickname_[PlayerNicknameLength];

    bool authFlag_{ false };
    bool duplicate_{ false };
};