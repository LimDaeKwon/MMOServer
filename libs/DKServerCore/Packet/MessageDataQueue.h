#pragma once

#include "CoreDefines.h"
#include "SpinLock.h"

struct MessageData;

class MessageDataQueue
{
public:
    MessageDataQueue();
    MessageDataQueue(int bufferSize);
    ~MessageDataQueue();

    void Resize(int newSize);

    int GetBufferSize() const;
    int GetUseSize() const;
    int GetFreeSize() const;
    int IsEmpty() const;

    bool Enqueue(MessageData* data);
    bool Dequeue(MessageData** out);

    void ClearBuffer();

private:
    MessageData** messageQueue_;
    int front_;
    int rear_;
    int size_;

    SpinLock messageQueueLock_;
};