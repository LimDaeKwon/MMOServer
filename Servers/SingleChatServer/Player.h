#pragma once

#include "GameDefine.h"
#include "Sector.h"

struct Player
{
    __int64 sessionId_;
    unsigned long long lastRecvTime_;
    __int64 accountNo_;

    SectorPosition sectorPosition_;

    wchar_t id_[PlayerIdLength];
    wchar_t nickname_[PlayerNicknameLength];

    bool authFlag_;
    bool duplicate_;
};