#include "NetworkProxy.h"

#include "CPacket.h"
#include "GameDefine.h"
#include "Network.h"
#include "PacketDefine.h"

void MakePacketMoveStart(SessionId sessionId, CPacket* packet, SessionId id, unsigned char direction, unsigned short x, unsigned short y)
{
    *packet << static_cast<unsigned char>(PacketCode) << static_cast<unsigned char>(9) << static_cast<unsigned char>(PacketScMoveStart) << id << direction << x << y;

    SendPacketAround(sessionId, packet);
}

void MakePacketMoveStartForMe(SessionId sessionId, CPacket* packet, SessionId id, unsigned char direction, unsigned short x, unsigned short y)
{
    *packet << static_cast<unsigned char>(PacketCode) << static_cast<unsigned char>(9) << static_cast<unsigned char>(PacketScMoveStart) << id << direction << x << y;

    SendPacketUnicast(sessionId, packet);
}

void MakePacketMoveStop(SessionId sessionId, CPacket* packet, SessionId id, unsigned char direction, unsigned short x, unsigned short y)
{
    *packet << static_cast<unsigned char>(PacketCode) << static_cast<unsigned char>(9) << static_cast<unsigned char>(PacketScMoveStop) << id << direction << x << y;

    SendPacketAround(sessionId, packet);
}

void MakePacketCreateMyCharacter(SessionId sessionId, CPacket* packet, SessionId id, unsigned char direction, unsigned short x, unsigned short y, unsigned char hp)
{
    *packet << static_cast<unsigned char>(PacketCode) << static_cast<unsigned char>(10) << static_cast<unsigned char>(PacketScCreateMyCharacter) << id << direction << x << y << hp;

    SendPacketUnicast(sessionId, packet);
}

void MakePacketCreateOtherCharacterForMe(SessionId sessionId, CPacket* packet, SessionId id, unsigned char direction, unsigned short x, unsigned short y, unsigned char hp)
{
    *packet << static_cast<unsigned char>(PacketCode) << static_cast<unsigned char>(10) << static_cast<unsigned char>(PacketScCreateOtherCharacter) << id << direction << x << y << hp;

    SendPacketUnicast(sessionId, packet);
}

void MakePacketCreateOtherCharacter(SessionId sessionId, CPacket* packet, SessionId id, unsigned char direction, unsigned short x, unsigned short y, unsigned char hp)
{
    *packet << static_cast<unsigned char>(PacketCode) << static_cast<unsigned char>(10) << static_cast<unsigned char>(PacketScCreateOtherCharacter) << id << direction << x << y << hp;

    SendPacketAround(sessionId, packet);
}

void MakePacketDeleteCharacter(SessionId sessionId, CPacket* packet, SessionId id)
{
    *packet << static_cast<unsigned char>(PacketCode) << static_cast<unsigned char>(4) << static_cast<unsigned char>(PacketScDeleteCharacter) << id;

    SendPacketAround(sessionId, packet);
}

void MakePacketDamage(SessionId sessionId, CPacket* packet, SessionId attackId, SessionId damageId, unsigned char damageHp)
{
    *packet << static_cast<unsigned char>(PacketCode) << static_cast<unsigned char>(9) << static_cast<unsigned char>(PacketScDamage) << attackId << damageId << damageHp;

    SendPacketAround(sessionId, packet, true);
}

void MakePacketAttack1(SessionId sessionId, CPacket* packet, SessionId id, unsigned char direction, unsigned short x, unsigned short y)
{
    *packet << static_cast<unsigned char>(PacketCode) << static_cast<unsigned char>(9) << static_cast<unsigned char>(PacketScAttack1) << id << direction << x << y;

    SendPacketAround(sessionId, packet);
}

void MakePacketAttack2(SessionId sessionId, CPacket* packet, SessionId id, unsigned char direction, unsigned short x, unsigned short y)
{
    *packet << static_cast<unsigned char>(PacketCode) << static_cast<unsigned char>(9) << static_cast<unsigned char>(PacketScAttack2) << id << direction << x << y;

    SendPacketAround(sessionId, packet);
}

void MakePacketAttack3(SessionId sessionId, CPacket* packet, SessionId id, unsigned char direction, unsigned short x, unsigned short y)
{
    *packet << static_cast<unsigned char>(PacketCode) << static_cast<unsigned char>(9) << static_cast<unsigned char>(PacketScAttack3) << id << direction << x << y;

    SendPacketAround(sessionId, packet);
}

void MakePacketEcho(SessionId sessionId, CPacket* packet, unsigned int time)
{
    *packet << static_cast<unsigned char>(PacketCode) << static_cast<unsigned char>(4) << static_cast<unsigned char>(PacketScEcho) << time;

    SendPacketUnicast(sessionId, packet);
}

void MakePacketDeleteCharacterRemoveSector(SessionId sessionId, CPacket* packet, SectorAround* around, SessionId id)
{
    *packet << static_cast<unsigned char>(PacketCode) << static_cast<unsigned char>(4) << static_cast<unsigned char>(PacketScDeleteCharacter) << id;

    SendPacketAroundRemoveSector(sessionId, packet, around);
}

void MakePacketDeleteCharacterForMe(SessionId sessionId, CPacket* packet, SessionId id)
{
    *packet << static_cast<unsigned char>(PacketCode) << static_cast<unsigned char>(4) << static_cast<unsigned char>(PacketScDeleteCharacter) << id;

    SendPacketUnicast(sessionId, packet);
}

void MakePacketCreateCharacterAddSector(SessionId sessionId, CPacket* packet, SectorAround* around, SessionId id, unsigned char direction, unsigned short x, unsigned short y, unsigned char hp)
{
    *packet << static_cast<unsigned char>(PacketCode) << static_cast<unsigned char>(10) << static_cast<unsigned char>(PacketScCreateOtherCharacter) << id << direction << x << y << hp;

    SendPacketAroundAddSector(sessionId, packet, around);
}

void MakePacketMoveStartAddSector(SessionId sessionId, CPacket* packet, SectorAround* around, SessionId id, unsigned char direction, unsigned short x, unsigned short y)
{
    *packet << static_cast<unsigned char>(PacketCode) << static_cast<unsigned char>(9) << static_cast<unsigned char>(PacketScMoveStart) << id << direction << x << y;

    SendPacketAroundAddSector(sessionId, packet, around);
}

void MakePacketSync(SessionId sessionId, CPacket* packet, SessionId id, unsigned short x, unsigned short y)
{
    *packet << static_cast<unsigned char>(PacketCode) << static_cast<unsigned char>(8) << static_cast<unsigned char>(PacketScSync) << id << x << y;

    SendPacketUnicast(sessionId, packet);
}