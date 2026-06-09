#pragma once

#include <WinSock2.h>
#include "RingBuffer.h"

struct Session
{
    unsigned int lastRecvTime_;
    unsigned __int64 sessionId_;

    SOCKET socket_;
    RingBuffer sendQueue_;
    RingBuffer receiveQueue_;

    unsigned int isDelete_;
};
