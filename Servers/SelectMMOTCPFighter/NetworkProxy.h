#pragma once 


struct SESSION;
struct SectorAround;
class CPacket;

void MakePacketMoveStart(SESSION* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y);


void MakePacketMoveStartForMe(SESSION* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y);


void MakePacketMoveStop(SESSION* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y);


void MakePacketCreateMyCharacter(SESSION* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y, unsigned char HP);


void MakePacketCreateOtherCharacterForMe(SESSION* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y, unsigned char HP);


void MakePacketCreateOtherCharacter(SESSION* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y, unsigned char HP);


void MakePacketDeleteCharacter(SESSION* Target, CPacket* Packet, unsigned int ID);


void MakePacketDamage(SESSION* Target, CPacket* Packet, unsigned int AttackID, unsigned int DamageID, unsigned char DamageHP);


void MakePacketAttack1(SESSION* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y);

void MakePacketAttack2(SESSION* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y);

void MakePacketAttack3(SESSION* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y);

void MakePacketEcho(SESSION* Target, CPacket* Packet, unsigned int Time);

void MakePacketDeleteCharacterRemoveSector(SESSION* Target, CPacket* Packet, SectorAround* Around, unsigned int ID);

void MakePacketDeleteCharacterForMe(SESSION* Target, CPacket* Packet, unsigned int ID);

void MakePacketCreateCharacterAddSector(SESSION* Target, CPacket* Packet, SectorAround* Around, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y, unsigned char HP);

void MakePacketMoveStartAddSector(SESSION* Target, CPacket* Packet, SectorAround* Around, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y);

void MakePacketSync(SESSION* Target, CPacket* Packet, unsigned int ID, unsigned short X, unsigned short Y);

