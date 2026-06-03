#pragma once

#include <WinSock2.h>
#include "RingBuffer.h"

struct Session
{
    unsigned int LastRecvTime;
    unsigned int SessionID;

    SOCKET Socket;
    RingBuffer SendQ;
    RingBuffer ReceiveQ;

    unsigned int IsDelete;
};