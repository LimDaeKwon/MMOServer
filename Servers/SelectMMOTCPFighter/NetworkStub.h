#pragma once

using SessionId = unsigned int;

struct Session;
class CPacket;

bool PacketProc(SessionId sessionId, unsigned char packetType, CPacket* packetBuffer);

bool NetPacketProcMoveStart(SessionId sessionId, unsigned char direction, unsigned short x, unsigned short y);

bool NetPacketProcMoveStop(SessionId sessionId, unsigned char direction, unsigned short x, unsigned short y);

bool NetPacketProcAttack1(SessionId sessionId, unsigned char direction, unsigned short x, unsigned short y);

bool NetPacketProcAttack2(SessionId sessionId, unsigned char direction, unsigned short x, unsigned short y);

bool NetPacketProcAttack3(SessionId sessionId, unsigned char direction, unsigned short x, unsigned short y);

bool NetPacketProcEcho(SessionId sessionId, unsigned int time);