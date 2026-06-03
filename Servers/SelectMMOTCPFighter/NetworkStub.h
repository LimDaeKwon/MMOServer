#pragma once 


struct Session;
class CPacket;

bool PacketProc(Session* Target, unsigned char PacketType, CPacket* PacketBuffer);

bool NetPacketProc_MoveStart(Session* Target, unsigned char Direction, unsigned short X, unsigned short Y);


bool NetPacketProc_MoveStop(Session* Target, unsigned char Direction, unsigned short X, unsigned short Y);


bool NetPacketProc_Attack1(Session* Target, unsigned char Direction, unsigned short X, unsigned short Y);


bool NetPacketProc_Attack2(Session* Target, unsigned char Direction, unsigned short X, unsigned short Y);


bool NetPacketProc_Attack3(Session* Target, unsigned char Direction, unsigned short X, unsigned short Y);


bool NetPacketProc_Echo(Session* Target, unsigned int Time);