#pragma once

struct fd_set;
struct Session;
struct SectorAround;
class CPacket;

using SessionId = unsigned int;


void Network();

void AcceptClient();

void SendPacketAround(SessionId Except, CPacket* Packet , bool SendMe = false);

void SendPacketUnicast(SessionId sessionId, CPacket* Packet);

void SendPacketSectorOne(int SectorX, int SectorY, SessionId Except, CPacket* Packet);

void SendPacketAroundRemoveSector(SessionId Target, CPacket* Packet, SectorAround* Around);

void SendPacketAroundAddSector(SessionId Target, CPacket* Packet, SectorAround* Around);

void Receive(Session* Target);

void ServerControl();

void SendAll(Session* Target);

void DeleteDisconnect();

void Initialize();

