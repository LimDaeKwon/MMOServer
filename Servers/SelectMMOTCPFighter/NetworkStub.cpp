#include "NetworkStub.h"
#include "CPacket.h" 


bool PacketProc(SESSION* Target, unsigned char PacketType, CPacket* Packet)
{
	switch (PacketType)
	{

	case 10:
	{
		unsigned char Direction;
		unsigned short X;
		unsigned short Y;
		*Packet >> Direction >> X >> Y;
		return NetPacketProc_MoveStart(Target, Direction, X, Y);
		break;
	}

	case 12:
	{
		unsigned char Direction;
		unsigned short X;
		unsigned short Y;
		*Packet >> Direction >> X >> Y;
		return NetPacketProc_MoveStop(Target, Direction, X, Y);
		break;
	}

	case 20:
	{
		unsigned char Direction;
		unsigned short X;
		unsigned short Y;
		*Packet >> Direction >> X >> Y;
		return NetPacketProc_Attack1(Target, Direction, X, Y);
		break;
	}

	case 22:
	{
		unsigned char Direction;
		unsigned short X;
		unsigned short Y;
		*Packet >> Direction >> X >> Y;
		return NetPacketProc_Attack2(Target, Direction, X, Y);
		break;
	}

	case 24:
	{
		unsigned char Direction;
		unsigned short X;
		unsigned short Y;
		*Packet >> Direction >> X >> Y;
		return NetPacketProc_Attack3(Target, Direction, X, Y);
		break;
	}

	case 252:
	{
		unsigned int Time;
		*Packet >> Time;
		return NetPacketProc_Echo(Target , Time);
		break;
	}

	}
	return true;
}
