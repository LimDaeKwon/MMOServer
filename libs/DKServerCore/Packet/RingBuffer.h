#pragma once

#include "CoreDefines.h"

class RingBuffer
{
public:
    RingBuffer();
    RingBuffer(int bufferSize);
    ~RingBuffer();

    void Resize(int newSize);

    int GetBufferSize() const;
    int GetUseSize() const;
    int GetFreeSize() const;

    int Enqueue(const char* data, int enqueueSize);
    int Dequeue(char* destination, int dequeueSize);
    int Peek(char* destination, int peekSize);

    void ClearBuffer();

    int MoveRear(int moveSize);
    int MoveFront(int moveSize);

    char* GetRearBufferPtr();
    char* GetStartBufferPtr();
    char* GetFrontBufferPtr();

    int DirectEnqueueSize() const;
    int DirectDequeueSize() const;

private:
    char* ringBuffer_;
    int front_;
    int rear_;
    int size_;
};