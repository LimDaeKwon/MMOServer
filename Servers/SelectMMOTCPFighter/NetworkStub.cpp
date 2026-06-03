#include "NetworkStub.h"
#include "CPacket.h" 


bool PacketProc(Session* Target, unsigned char PacketType, CPacket* Packet)
{
	switch (PacketType)
	{

	case 10:
	{
		unsigned char Direction;
		unsigned short X;
		unsigned short Y;
		*Packet >> Direction >> X >> Y;
		return NetPacketProcMoveStart(Target, Direction, X, Y);
		break;
	}

	case 12:
	{
		unsigned char Direction;
		unsigned short X;
		unsigned short Y;
		*Packet >> Direction >> X >> Y;
		return NetPacketProcMoveStop(Target, Direction, X, Y);
		break;
	}

	case 20:
	{
		unsigned char Direction;
		unsigned short X;
		unsigned short Y;
		*Packet >> Direction >> X >> Y;
		return NetPacketProcAttack1(Target, Direction, X, Y);
		break;
	}

	case 22:
	{
		unsigned char Direction;
		unsigned short X;
		unsigned short Y;
		*Packet >> Direction >> X >> Y;
		return NetPacketProcAttack2(Target, Direction, X, Y);
		break;
	}

	case 24:
	{
		unsigned char Direction;
		unsigned short X;
		unsigned short Y;
		*Packet >> Direction >> X >> Y;
		return NetPacketProcAttack3(Target, Direction, X, Y);
		break;
	}

	case 252:
	{
		unsigned int Time;
		*Packet >> Time;
		return NetPacketProcEcho(Target , Time);
		break;
	}

	}
	return true;
}
