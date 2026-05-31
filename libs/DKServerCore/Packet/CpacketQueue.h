#pragma once

#include "CoreDefines.h"

class CPacket;

class CPacketQueue
{
public:
    CPacketQueue();
    CPacketQueue(int bufferSize);
    ~CPacketQueue();

    void Resize(int newSize);

    int GetBufferSize() const;
    int GetUseSize() const;
    int GetFreeSize() const;
    int IsEmpty() const;

    bool Enqueue(CPacket* data);
    bool Dequeue(CPacket** out);

    void ClearBuffer();

private:
    CPacket** packetQueue_;
    int front_;
    int rear_;
    int size_;
};