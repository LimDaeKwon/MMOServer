#include "NetworkStub.h"

#include "CPacket.h"
#include "PacketDefine.h"

using SessionId = unsigned int;


bool PacketProc(SessionId sessionId, unsigned char packetType, CPacket* packetBuffer)
{
    switch (packetType)
    {
    case PacketCsMoveStart:
    {
        unsigned char direction;
        unsigned short x;
        unsigned short y;

        *packetBuffer >> direction >> x >> y;

        return NetPacketProcMoveStart(sessionId, direction, x, y);
    }

    case PacketCsMoveStop:
    {
        unsigned char direction;
        unsigned short x;
        unsigned short y;

        *packetBuffer >> direction >> x >> y;

        return NetPacketProcMoveStop(sessionId, direction, x, y);
    }

    case PacketCsAttack1:
    {
        unsigned char direction;
        unsigned short x;
        unsigned short y;

        *packetBuffer >> direction >> x >> y;

        return NetPacketProcAttack1(sessionId, direction, x, y);
    }

    case PacketCsAttack2:
    {
        unsigned char direction;
        unsigned short x;
        unsigned short y;

        *packetBuffer >> direction >> x >> y;

        return NetPacketProcAttack2(sessionId, direction, x, y);
    }

    case PacketCsAttack3:
    {
        unsigned char direction;
        unsigned short x;
        unsigned short y;

        *packetBuffer >> direction >> x >> y;

        return NetPacketProcAttack3(sessionId, direction, x, y);
    }

    case PacketCsEcho:
    {
        unsigned int time;

        *packetBuffer >> time;

        return NetPacketProcEcho(sessionId, time);
    }

    default:
    {
        break;
    }
    }

    return true;
}