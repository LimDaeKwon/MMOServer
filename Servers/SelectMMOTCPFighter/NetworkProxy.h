#pragma once

struct SectorAround;
class CPacket;

using SessionId = unsigned int;

void MakePacketMoveStart(SessionId sessionId, CPacket* packet, SessionId id, unsigned char direction, unsigned short x, unsigned short y);

void MakePacketMoveStartForMe(SessionId sessionId, CPacket* packet, SessionId id, unsigned char direction, unsigned short x, unsigned short y);

void MakePacketMoveStop(SessionId sessionId, CPacket* packet, SessionId id, unsigned char direction, unsigned short x, unsigned short y);

void MakePacketCreateMyCharacter(SessionId sessionId, CPacket* packet, SessionId id, unsigned char direction, unsigned short x, unsigned short y, unsigned char hp);

void MakePacketCreateOtherCharacterForMe(SessionId sessionId, CPacket* packet, SessionId id, unsigned char direction, unsigned short x, unsigned short y, unsigned char hp);

void MakePacketCreateOtherCharacter(SessionId sessionId, CPacket* packet, SessionId id, unsigned char direction, unsigned short x, unsigned short y, unsigned char hp);

void MakePacketDeleteCharacter(SessionId sessionId, CPacket* packet, SessionId id);

void MakePacketDamage(SessionId sessionId, CPacket* packet, SessionId attackId, SessionId damageId, unsigned char damageHp);

void MakePacketAttack1(SessionId sessionId, CPacket* packet, SessionId id, unsigned char direction, unsigned short x, unsigned short y);

void MakePacketAttack2(SessionId sessionId, CPacket* packet, SessionId id, unsigned char direction, unsigned short x, unsigned short y);

void MakePacketAttack3(SessionId sessionId, CPacket* packet, SessionId id, unsigned char direction, unsigned short x, unsigned short y);

void MakePacketEcho(SessionId sessionId, CPacket* packet, unsigned int time);

void MakePacketDeleteCharacterRemoveSector(SessionId sessionId, CPacket* packet, SectorAround* around, SessionId id);

void MakePacketDeleteCharacterForMe(SessionId sessionId, CPacket* packet, SessionId id);

void MakePacketCreateCharacterAddSector(SessionId sessionId, CPacket* packet, SectorAround* around, SessionId id, unsigned char direction, unsigned short x, unsigned short y, unsigned char hp);

void MakePacketMoveStartAddSector(SessionId sessionId, CPacket* packet, SectorAround* around, SessionId id, unsigned char direction, unsigned short x, unsigned short y);

void MakePacketSync(SessionId sessionId, CPacket* packet, SessionId id, unsigned short x, unsigned short y);