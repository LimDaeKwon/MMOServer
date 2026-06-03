#pragma once

struct Session;
class CPacket;

bool PacketProc(Session* target, unsigned char packetType, CPacket* packetBuffer);

bool NetPacketProcMoveStart(Session* target, unsigned char direction, unsigned short x, unsigned short y);

bool NetPacketProcMoveStop(Session* target, unsigned char direction, unsigned short x, unsigned short y);

bool NetPacketProcAttack1(Session* target, unsigned char direction, unsigned short x, unsigned short y);

bool NetPacketProcAttack2(Session* target, unsigned char direction, unsigned short x, unsigned short y);

bool NetPacketProcAttack3(Session* target, unsigned char direction, unsigned short x, unsigned short y);

bool NetPacketProcEcho(Session* target, unsigned int time);