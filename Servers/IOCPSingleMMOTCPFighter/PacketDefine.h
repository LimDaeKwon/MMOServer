#pragma once

#pragma pack(push, 1)

constexpr int RangeMoveTop = 0;
constexpr int RangeMoveLeft = 0;
constexpr int RangeMoveRight = 6400;
constexpr int RangeMoveBottom = 6400;
constexpr int ErrorRange = 50;
constexpr int NetworkPacketRecvTimeout = 30000;

constexpr int Attack1RangeX = 80;
constexpr int Attack2RangeX = 90;
constexpr int Attack3RangeX = 100;
constexpr int Attack1RangeY = 10;
constexpr int Attack2RangeY = 10;
constexpr int Attack3RangeY = 20;

constexpr int Attack1Damage = 1;
constexpr int Attack2Damage = 2;
constexpr int Attack3Damage = 3;

constexpr int SectorMaxX = 40;
constexpr int SectorMaxY = 40;
constexpr int SectorXSize = 160;
constexpr int SectorYSize = 160;

constexpr unsigned short ServerPort = 20511;
constexpr unsigned char PacketCode = 0x89;
constexpr unsigned char DefaultHp = 100;
constexpr int NetworkPacketHeaderSize = 3;
constexpr unsigned char FixedUpdateFrameMs = 40;

constexpr unsigned char PacketMoveDirectionLL = 0;
constexpr unsigned char PacketMoveDirectionLU = 1;
constexpr unsigned char PacketMoveDirectionUU = 2;
constexpr unsigned char PacketMoveDirectionRU = 3;
constexpr unsigned char PacketMoveDirectionRR = 4;
constexpr unsigned char PacketMoveDirectionRD = 5;
constexpr unsigned char PacketMoveDirectionDD = 6;
constexpr unsigned char PacketMoveDirectionLD = 7;

constexpr unsigned char PacketScCreateMyCharacter = 0;
constexpr unsigned char PacketScCreateOtherCharacter = 1;
constexpr unsigned char PacketScDeleteCharacter = 2;
constexpr unsigned char PacketCsMoveStart = 10;
constexpr unsigned char PacketScMoveStart = 11;
constexpr unsigned char PacketCsMoveStop = 12;
constexpr unsigned char PacketScMoveStop = 13;
constexpr unsigned char PacketCsAttack1 = 20;
constexpr unsigned char PacketScAttack1 = 21;
constexpr unsigned char PacketCsAttack2 = 22;
constexpr unsigned char PacketScAttack2 = 23;
constexpr unsigned char PacketCsAttack3 = 24;
constexpr unsigned char PacketScAttack3 = 25;
constexpr unsigned char PacketScDamage = 30;
constexpr unsigned char PacketCsSync = 250;
constexpr unsigned char PacketScSync = 251;
constexpr unsigned char PacketCsEcho = 252;
constexpr unsigned char PacketScEcho = 253;

struct PacketHeader
{
	unsigned char code_;
	unsigned char size_;
	unsigned char type_;
};

struct PacketScCreateMyCharacterData
{
	unsigned int id_;
	unsigned char direction_;
	unsigned short x_;
	unsigned short y_;
	unsigned char hp_;
};

struct PacketScCreateOtherCharacterData
{
	unsigned int id_;
	unsigned char direction_;
	unsigned short x_;
	unsigned short y_;
	unsigned char hp_;
};

struct PacketScDeleteCharacterData
{
	unsigned int id_;
};

struct PacketCsMoveStartData
{
	unsigned char direction_;
	unsigned short x_;
	unsigned short y_;
};

struct PacketScMoveStartData
{
	unsigned int id_;
	unsigned char direction_;
	unsigned short x_;
	unsigned short y_;
};

struct PacketCsMoveStopData
{
	unsigned char direction_;
	unsigned short x_;
	unsigned short y_;
};

struct PacketScMoveStopData
{
	unsigned int id_;
	unsigned char direction_;
	unsigned short x_;
	unsigned short y_;
};

struct PacketCsAttackData
{
	unsigned char direction_;
	unsigned short x_;
	unsigned short y_;
};

struct PacketScAttackData
{
	unsigned int id_;
	unsigned char direction_;
	unsigned short x_;
	unsigned short y_;
};

struct PacketScDamageData
{
	unsigned int attackId_;
	unsigned int damageId_;
	unsigned char damageHp_;
};

struct PacketCsSyncData
{
	unsigned short x_;
	unsigned short y_;
};

struct PacketScSyncData
{
	unsigned int id_;
	unsigned short x_;
	unsigned short y_;
};

#pragma pack(pop)
