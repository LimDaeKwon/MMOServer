#pragma once 


struct Session;
struct SectorAround;
class CPacket;

void MakePacketMoveStart(Session* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y);


void MakePacketMoveStartForMe(Session* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y);


void MakePacketMoveStop(Session* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y);


void MakePacketCreateMyCharacter(Session* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y, unsigned char HP);


void MakePacketCreateOtherCharacterForMe(Session* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y, unsigned char HP);


void MakePacketCreateOtherCharacter(Session* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y, unsigned char HP);


void MakePacketDeleteCharacter(Session* Target, CPacket* Packet, unsigned int ID);


void MakePacketDamage(Session* Target, CPacket* Packet, unsigned int AttackID, unsigned int DamageID, unsigned char DamageHP);


void MakePacketAttack1(Session* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y);

void MakePacketAttack2(Session* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y);

void MakePacketAttack3(Session* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y);

void MakePacketEcho(Session* Target, CPacket* Packet, unsigned int Time);

void MakePacketDeleteCharacterRemoveSector(Session* Target, CPacket* Packet, SectorAround* Around, unsigned int ID);

void MakePacketDeleteCharacterForMe(Session* Target, CPacket* Packet, unsigned int ID);

void MakePacketCreateCharacterAddSector(Session* Target, CPacket* Packet, SectorAround* Around, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y, unsigned char HP);

void MakePacketMoveStartAddSector(Session* Target, CPacket* Packet, SectorAround* Around, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y);

void MakePacketSync(Session* Target, CPacket* Packet, unsigned int ID, unsigned short X, unsigned short Y);

