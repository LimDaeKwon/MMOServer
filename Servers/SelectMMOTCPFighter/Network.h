#pragma once

struct Session;
class CPacket;

using SessionId = unsigned int;

void Network();

void AcceptClient();

void SendPacket(SessionId sessionId, CPacket* packet);

void Receive(Session* target);

void ServerControl();

void SendAll(Session* target);

void Disconnect(SessionId sessionId);

void DeleteDisconnect();

void Initialize();

void TimeOut();