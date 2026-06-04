#pragma once

struct fd_set;
struct Session;
struct SectorAround;
class CPacket;

using SessionId = unsigned int;


void Network();

void AcceptClient();

void SendPacket(SessionId sessionId, CPacket* Packet);

void Receive(Session* Target);

void ServerControl();

void SendAll(Session* Target);

void DeleteDisconnect();

void Initialize();

