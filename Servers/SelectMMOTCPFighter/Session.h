#pragma once

#include <WinSock2.h>
#include "RingBuffer.h"

struct Session
{
    unsigned int lastRecvTime_;
    unsigned int sessionId_;

    SOCKET socket_;
    RingBuffer sendQueue_;
    RingBuffer receiveQueue_;

    unsigned int isDelete_;
};
