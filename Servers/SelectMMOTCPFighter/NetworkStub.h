#pragma once 


struct SESSION;
class CPacket;

bool PacketProc(SESSION* Target, unsigned char PacketType, CPacket* PacketBuffer);

bool NetPacketProc_MoveStart(SESSION* Target, unsigned char Direction, unsigned short X, unsigned short Y);


bool NetPacketProc_MoveStop(SESSION* Target, unsigned char Direction, unsigned short X, unsigned short Y);


bool NetPacketProc_Attack1(SESSION* Target, unsigned char Direction, unsigned short X, unsigned short Y);


bool NetPacketProc_Attack2(SESSION* Target, unsigned char Direction, unsigned short X, unsigned short Y);


bool NetPacketProc_Attack3(SESSION* Target, unsigned char Direction, unsigned short X, unsigned short Y);


bool NetPacketProc_Echo(SESSION* Target, unsigned int Time);