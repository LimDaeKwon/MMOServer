#pragma once



#define SERVERPORT 20511
#define PACKETCODE 0x89
#define DEFAULTHP 100

struct fd_set;
struct SESSION;
struct SectorAround;
class CPacket;


void Network();

void AcceptClient();

void SendPacketAround(SESSION* Except, CPacket* Packet , bool SendMe = false);

void SendPacketUnicast(SESSION* Target, CPacket* Packet);

void SendPacketSectorOne(int SectorX, int SectorY, SESSION* Except, CPacket* Packet);

void SendPacketAroundRemoveSector(SESSION* Target, CPacket* Packet, SectorAround* Around);

void SendPacketAroundAddSector(SESSION* Target, CPacket* Packet, SectorAround* Around);

void Receive(SESSION* Target);

void ServerControl();

void SendAll(SESSION* Target);

void DeleteDisconnect();

void Initialize();

