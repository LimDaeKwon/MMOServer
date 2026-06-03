#include "NetworkProxy.h"

#include "CPacket.h"
#include "GameDefine.h"
#include "Network.h"
#include "PacketDefine.h"

void MakePacketMoveStart(Session* target, CPacket* packet, unsigned int id, unsigned char direction, unsigned short x, unsigned short y)
{
    *packet << static_cast<unsigned char>(PacketCode) << static_cast<unsigned char>(9) << static_cast<unsigned char>(PacketScMoveStart) << id << direction << x << y;

    SendPacketAround(target, packet);
}

void MakePacketMoveStartForMe(Session* target, CPacket* packet, unsigned int id, unsigned char direction, unsigned short x, unsigned short y)
{
    *packet << static_cast<unsigned char>(PacketCode) << static_cast<unsigned char>(9) << static_cast<unsigned char>(PacketScMoveStart) << id << direction << x << y;

    SendPacketUnicast(target, packet);
}

void MakePacketMoveStop(Session* target, CPacket* packet, unsigned int id, unsigned char direction, unsigned short x, unsigned short y)
{
    *packet << static_cast<unsigned char>(PacketCode) << static_cast<unsigned char>(9) << static_cast<unsigned char>(PacketScMoveStop) << id << direction << x << y;

    SendPacketAround(target, packet);
}

void MakePacketCreateMyCharacter(Session* target, CPacket* packet, unsigned int id, unsigned char direction, unsigned short x, unsigned short y, unsigned char hp)
{
    *packet << static_cast<unsigned char>(PacketCode) << static_cast<unsigned char>(10) << static_cast<unsigned char>(PacketScCreateMyCharacter) << id << direction << x << y << hp;

    SendPacketUnicast(target, packet);
}

void MakePacketCreateOtherCharacterForMe(Session* target, CPacket* packet, unsigned int id, unsigned char direction, unsigned short x, unsigned short y, unsigned char hp)
{
    *packet << static_cast<unsigned char>(PacketCode) << static_cast<unsigned char>(10) << static_cast<unsigned char>(PacketScCreateOtherCharacter) << id << direction << x << y << hp;

    SendPacketUnicast(target, packet);
}

void MakePacketCreateOtherCharacter(Session* target, CPacket* packet, unsigned int id, unsigned char direction, unsigned short x, unsigned short y, unsigned char hp)
{
    *packet << static_cast<unsigned char>(PacketCode) << static_cast<unsigned char>(10) << static_cast<unsigned char>(PacketScCreateOtherCharacter) << id << direction << x << y << hp;

    SendPacketAround(target, packet);
}

void MakePacketDeleteCharacter(Session* target, CPacket* packet, unsigned int id)
{
    *packet << static_cast<unsigned char>(PacketCode) << static_cast<unsigned char>(4) << static_cast<unsigned char>(PacketScDeleteCharacter) << id;

    SendPacketAround(target, packet);
}

void MakePacketDamage(Session* target, CPacket* packet, unsigned int attackId, unsigned int damageId, unsigned char damageHp)
{
    *packet << static_cast<unsigned char>(PacketCode) << static_cast<unsigned char>(9) << static_cast<unsigned char>(PacketScDamage) << attackId << damageId << damageHp;

    SendPacketAround(target, packet, true);
}

void MakePacketAttack1(Session* target, CPacket* packet, unsigned int id, unsigned char direction, unsigned short x, unsigned short y)
{
    *packet << static_cast<unsigned char>(PacketCode) << static_cast<unsigned char>(9) << static_cast<unsigned char>(PacketScAttack1) << id << direction << x << y;

    SendPacketAround(target, packet);
}

void MakePacketAttack2(Session* target, CPacket* packet, unsigned int id, unsigned char direction, unsigned short x, unsigned short y)
{
    *packet << static_cast<unsigned char>(PacketCode) << static_cast<unsigned char>(9) << static_cast<unsigned char>(PacketScAttack2) << id << direction << x << y;

    SendPacketAround(target, packet);
}

void MakePacketAttack3(Session* target, CPacket* packet, unsigned int id, unsigned char direction, unsigned short x, unsigned short y)
{
    *packet << static_cast<unsigned char>(PacketCode) << static_cast<unsigned char>(9) << static_cast<unsigned char>(PacketScAttack3) << id << direction << x << y;

    SendPacketAround(target, packet);
}

void MakePacketEcho(Session* target, CPacket* packet, unsigned int time)
{
    *packet << static_cast<unsigned char>(PacketCode) << static_cast<unsigned char>(4) << static_cast<unsigned char>(PacketScEcho) << time;

    SendPacketUnicast(target, packet);
}

void MakePacketDeleteCharacterRemoveSector(Session* target, CPacket* packet, SectorAround* around, unsigned int id)
{
    *packet << static_cast<unsigned char>(PacketCode) << static_cast<unsigned char>(4) << static_cast<unsigned char>(PacketScDeleteCharacter) << id;

    SendPacketAroundRemoveSector(target, packet, around);
}

void MakePacketCreateCharacterAddSector(Session* target, CPacket* packet, SectorAround* around, unsigned int id, unsigned char direction, unsigned short x, unsigned short y, unsigned char hp)
{
    *packet << static_cast<unsigned char>(PacketCode) << static_cast<unsigned char>(10) << static_cast<unsigned char>(PacketScCreateOtherCharacter) << id << direction << x << y << hp;

    SendPacketAroundAddSector(target, packet, around);
}

void MakePacketMoveStartAddSector(Session* target, CPacket* packet, SectorAround* around, unsigned int id, unsigned char direction, unsigned short x, unsigned short y)
{
    *packet << static_cast<unsigned char>(PacketCode) << static_cast<unsigned char>(9) << static_cast<unsigned char>(PacketScMoveStart) << id << direction << x << y;

    SendPacketAroundAddSector(target, packet, around);
}

void MakePacketDeleteCharacterForMe(Session* target, CPacket* packet, unsigned int id)
{
    *packet << static_cast<unsigned char>(PacketCode) << static_cast<unsigned char>(4) << static_cast<unsigned char>(PacketScDeleteCharacter) << id;

    SendPacketUnicast(target, packet);
}

void MakePacketSync(Session* target, CPacket* packet, unsigned int id, unsigned short x, unsigned short y)
{
    *packet << static_cast<unsigned char>(PacketCode) << static_cast<unsigned char>(8) << static_cast<unsigned char>(PacketScSync) << id << x << y;

    SendPacketAround(target, packet, true);
}