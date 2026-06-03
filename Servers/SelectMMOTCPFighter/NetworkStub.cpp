#include "NetworkStub.h"

#include "CPacket.h"
#include "PacketDefine.h"

bool PacketProc(Session* target, unsigned char packetType, CPacket* packetBuffer)
{
    switch (packetType)
    {
    case PacketCsMoveStart:
    {
        unsigned char direction;
        unsigned short x;
        unsigned short y;

        *packetBuffer >> direction >> x >> y;

        return NetPacketProcMoveStart(target, direction, x, y);
    }

    case PacketCsMoveStop:
    {
        unsigned char direction;
        unsigned short x;
        unsigned short y;

        *packetBuffer >> direction >> x >> y;

        return NetPacketProcMoveStop(target, direction, x, y);
    }

    case PacketCsAttack1:
    {
        unsigned char direction;
        unsigned short x;
        unsigned short y;

        *packetBuffer >> direction >> x >> y;

        return NetPacketProcAttack1(target, direction, x, y);
    }

    case PacketCsAttack2:
    {
        unsigned char direction;
        unsigned short x;
        unsigned short y;

        *packetBuffer >> direction >> x >> y;

        return NetPacketProcAttack2(target, direction, x, y);
    }

    case PacketCsAttack3:
    {
        unsigned char direction;
        unsigned short x;
        unsigned short y;

        *packetBuffer >> direction >> x >> y;

        return NetPacketProcAttack3(target, direction, x, y);
    }

    case PacketCsEcho:
    {
        unsigned int time;

        *packetBuffer >> time;

        return NetPacketProcEcho(target, time);
    }

    default:
    {
        break;
    }
    }

    return true;
}