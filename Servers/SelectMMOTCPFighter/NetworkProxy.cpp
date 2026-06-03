#include "NetworkProxy.h"
#include "Network.h"
#include "CPacket.h" 


void MakePacketMoveStart(Session* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y)
{

	*Packet << (unsigned char)PACKETCODE << (unsigned char)9 << (unsigned char)11 << ID << Direction << X << Y;


	SendPacketAround(Target, Packet);
}


void MakePacketMoveStartForMe(Session* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y)
{

	*Packet << (unsigned char)PACKETCODE << (unsigned char)9 << (unsigned char)11 << ID << Direction << X << Y;


	SendPacketUnicast(Target, Packet);
}


void MakePacketMoveStop(Session* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y)
{

	*Packet << (unsigned char)PACKETCODE << (unsigned char)9 << (unsigned char)13 << ID << Direction << X << Y;


	SendPacketAround(Target, Packet);
}


void MakePacketCreateMyCharacter(Session* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y, unsigned char HP)
{

	*Packet << (unsigned char)PACKETCODE << (unsigned char)10 << (unsigned char)0 << ID << Direction << X << Y << HP;


	SendPacketUnicast(Target, Packet);
}


void MakePacketCreateOtherCharacterForMe(Session* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y, unsigned char HP)
{

	*Packet << (unsigned char)PACKETCODE << (unsigned char)10 << (unsigned char)1 << ID << Direction << X << Y << HP;


	SendPacketUnicast(Target, Packet);
}


void MakePacketCreateOtherCharacter(Session* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y, unsigned char HP)
{

	*Packet << (unsigned char)PACKETCODE << (unsigned char)10 << (unsigned char)1 << ID << Direction << X << Y << HP;


	SendPacketAround(Target, Packet);
}


void MakePacketDeleteCharacter(Session* Target, CPacket* Packet, unsigned int ID)
{

	*Packet << (unsigned char)PACKETCODE << (unsigned char)4 << (unsigned char)2 << ID;


	SendPacketAround(Target, Packet);
}


void MakePacketDamage(Session* Target, CPacket* Packet, unsigned int AttackID, unsigned int DamageID, unsigned char DamageHP)
{

	*Packet << (unsigned char)PACKETCODE << (unsigned char)9 << (unsigned char)30 << AttackID << DamageID << DamageHP;


	SendPacketAround(Target, Packet,true);
}


void MakePacketAttack1(Session* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y)
{

	*Packet << (unsigned char)PACKETCODE << (unsigned char)9 << (unsigned char)21 << ID << Direction << X << Y;


	SendPacketAround(Target, Packet);
}


void MakePacketAttack2(Session* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y)
{

	*Packet << (unsigned char)PACKETCODE << (unsigned char)9 << (unsigned char)23 << ID << Direction << X << Y;


	SendPacketAround(Target, Packet);
}


void MakePacketAttack3(Session* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y)
{

	*Packet << (unsigned char)PACKETCODE << (unsigned char)9 << (unsigned char)25 << ID << Direction << X << Y;


	SendPacketAround(Target, Packet);
}

void MakePacketEcho(Session* Target, CPacket* Packet , unsigned int Time)
{

	*Packet << (unsigned char)PACKETCODE << (unsigned char)4 << (unsigned char)253 << Time ;


	SendPacketUnicast(Target, Packet);
}

void MakePacketDeleteCharacterRemoveSector(Session* Target, CPacket* Packet, SectorAround* Around, unsigned int ID)
{
	*Packet << (unsigned char)PACKETCODE << (unsigned char)4 << (unsigned char)2 << ID;

	SendPacketAroundRemoveSector(Target, Packet, Around);
}

void MakePacketCreateCharacterAddSector(Session* Target, CPacket* Packet, SectorAround* Around, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y, unsigned char HP)
{
	*Packet << (unsigned char)PACKETCODE << (unsigned char)10 << (unsigned char)1 << ID << Direction << X << Y << HP;

	SendPacketAroundAddSector(Target, Packet, Around);
}

void MakePacketMoveStartAddSector(Session* Target, CPacket* Packet, SectorAround* Around, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y)
{
	*Packet << (unsigned char)PACKETCODE << (unsigned char)9 << (unsigned char)11 << ID << Direction << X << Y;


	SendPacketAroundAddSector(Target, Packet, Around);
}

void MakePacketDeleteCharacterForMe(Session* Target, CPacket* Packet, unsigned int ID)
{
	*Packet << (unsigned char)PACKETCODE << (unsigned char)4 << (unsigned char)2 << ID;


	SendPacketUnicast(Target, Packet);

}

void MakePacketSync(Session* Target, CPacket* Packet, unsigned int ID, unsigned short X, unsigned short Y)
{

	*Packet << (unsigned char)PACKETCODE << (unsigned char)8 << (unsigned char)251 << ID << X << Y;


	SendPacketAround(Target, Packet, true);
}
