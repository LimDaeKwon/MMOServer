#pragma once



struct fd_set;
struct Session;
struct SectorAround;
class CPacket;


void Network();

void AcceptClient();

void SendPacketAround(Session* Except, CPacket* Packet , bool SendMe = false);

void SendPacketUnicast(Session* Target, CPacket* Packet);

void SendPacketSectorOne(int SectorX, int SectorY, Session* Except, CPacket* Packet);

void SendPacketAroundRemoveSector(Session* Target, CPacket* Packet, SectorAround* Around);

void SendPacketAroundAddSector(Session* Target, CPacket* Packet, SectorAround* Around);

void Receive(Session* Target);

void ServerControl();

void SendAll(Session* Target);

void DeleteDisconnect();

void Initialize();

