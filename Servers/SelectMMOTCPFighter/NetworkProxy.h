#pragma once

struct Session;
struct SectorAround;
class CPacket;

void MakePacketMoveStart(Session* target, CPacket* packet, unsigned int id, unsigned char direction, unsigned short x, unsigned short y);

void MakePacketMoveStartForMe(Session* target, CPacket* packet, unsigned int id, unsigned char direction, unsigned short x, unsigned short y);

void MakePacketMoveStop(Session* target, CPacket* packet, unsigned int id, unsigned char direction, unsigned short x, unsigned short y);

void MakePacketCreateMyCharacter(Session* target, CPacket* packet, unsigned int id, unsigned char direction, unsigned short x, unsigned short y, unsigned char hp);

void MakePacketCreateOtherCharacterForMe(Session* target, CPacket* packet, unsigned int id, unsigned char direction, unsigned short x, unsigned short y, unsigned char hp);

void MakePacketCreateOtherCharacter(Session* target, CPacket* packet, unsigned int id, unsigned char direction, unsigned short x, unsigned short y, unsigned char hp);

void MakePacketDeleteCharacter(Session* target, CPacket* packet, unsigned int id);

void MakePacketDamage(Session* target, CPacket* packet, unsigned int attackId, unsigned int damageId, unsigned char damageHp);

void MakePacketAttack1(Session* target, CPacket* packet, unsigned int id, unsigned char direction, unsigned short x, unsigned short y);

void MakePacketAttack2(Session* target, CPacket* packet, unsigned int id, unsigned char direction, unsigned short x, unsigned short y);

void MakePacketAttack3(Session* target, CPacket* packet, unsigned int id, unsigned char direction, unsigned short x, unsigned short y);

void MakePacketEcho(Session* target, CPacket* packet, unsigned int time);

void MakePacketDeleteCharacterRemoveSector(Session* target, CPacket* packet, SectorAround* around, unsigned int id);

void MakePacketDeleteCharacterForMe(Session* target, CPacket* packet, unsigned int id);

void MakePacketCreateCharacterAddSector(Session* target, CPacket* packet, SectorAround* around, unsigned int id, unsigned char direction, unsigned short x, unsigned short y, unsigned char hp);

void MakePacketMoveStartAddSector(Session* target, CPacket* packet, SectorAround* around, unsigned int id, unsigned char direction, unsigned short x, unsigned short y);

void MakePacketSync(Session* target, CPacket* packet, unsigned int id, unsigned short x, unsigned short y);