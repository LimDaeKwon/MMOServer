#pragma once




#pragma pack(push, 1)

struct PacketHeader
{
    unsigned char byCode_;
    unsigned char bySize_;
    unsigned char byType_;
};

#pragma pack(pop)


constexpr int RangeMoveTop = 0;
constexpr int RangeMoveLeft = 0;
constexpr int RangeMoveRight = 6400;
constexpr int RangeMoveBottom = 6400;

constexpr int ErrorRange = 50;

constexpr int Attack1RangeX = 80;
constexpr int Attack2RangeX = 90;
constexpr int Attack3RangeX = 100;

constexpr int Attack1RangeY = 10;
constexpr int Attack2RangeY = 10;
constexpr int Attack3RangeY = 20;

constexpr int Attack1Damage = 1;
constexpr int Attack2Damage = 2;
constexpr int Attack3Damage = 3;

constexpr int SectorMaxX = 32;
constexpr int SectorMaxY = 40;
constexpr int SectorXSize = 200;
constexpr int SectorYSize = 160;

constexpr int DefaultHp = 100;

constexpr int ProfileOver = 12000;

constexpr unsigned char PacketMoveDirectionLL = 0;
constexpr unsigned char PacketMoveDirectionLU = 1;
constexpr unsigned char PacketMoveDirectionUU = 2;
constexpr unsigned char PacketMoveDirectionRU = 3;
constexpr unsigned char PacketMoveDirectionRR = 4;
constexpr unsigned char PacketMoveDirectionRD = 5;
constexpr unsigned char PacketMoveDirectionDD = 6;
constexpr unsigned char PacketMoveDirectionLD = 7;

constexpr int NetworkPacketRecvTimeout = 30000;
constexpr unsigned char PacketCode = 0x89;
constexpr int NetworkPacketHeaderSize = 3;
constexpr int LibraryHeaderSize = 5;
constexpr unsigned short ServerPort = 25000;
constexpr unsigned int DefaultMaxSessionCount = 10000;
constexpr unsigned __int64 InvalidSessionId = static_cast<unsigned __int64>(-1);
