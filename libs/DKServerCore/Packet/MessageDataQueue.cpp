#include "MessageDataQueue.h"

MessageDataQueue::MessageDataQueue()
    : messageQueue_(nullptr),
    front_(0),
    rear_(0),
    size_(DKServerCore::MessageDataQueueDefaultBufferSize)
{
    messageQueue_ = new MessageData * [DKServerCore::MessageDataQueueDefaultBufferSize];
}

MessageDataQueue::MessageDataQueue(int bufferSize)
    : messageQueue_(nullptr),
    front_(0),
    rear_(0),
    size_(bufferSize)
{
    messageQueue_ = new MessageData * [bufferSize];
}

MessageDataQueue::~MessageDataQueue()
{
    delete[] messageQueue_;
}

void MessageDataQueue::Resize(int newSize)
{
}

int MessageDataQueue::GetBufferSize() const
{
    return size_;
}

int MessageDataQueue::GetUseSize() const
{
    int localFront = front_;
    int localRear = rear_;

    if (localFront > localRear)
    {
        return size_ - localFront + localRear;
    }

    return localRear - localFront;
}

int MessageDataQueue::GetFreeSize() const
{
    return size_ - GetUseSize() - 1;
}

int MessageDataQueue::IsEmpty() const
{
    return rear_ == front_;
}

bool MessageDataQueue::Enqueue(MessageData* data)
{
    messageQueueLock_.Lock();

    if (GetFreeSize() == 0)
    {
        messageQueueLock_.Unlock();
        return false;
    }

    messageQueue_[rear_] = data;

    if (rear_ == size_ - 1)
    {
        rear_ = 0;
    }
    else
    {
        ++rear_;
    }

    messageQueueLock_.Unlock();

    return true;
}

bool MessageDataQueue::Dequeue(MessageData** out)
{
    //Dequeue는 하나의 스레드에서만 진행 할 목적이므로 락을 걸지 않음.
    if (out == nullptr)
    {
        return false;
    }

    if (IsEmpty())
    {
        return false;
    }

    *out = messageQueue_[front_];

    if (front_ == size_ - 1)
    {
        front_ = 0;
    }
    else
    {
        ++front_;
    }

    return true;
}

void MessageDataQueue::ClearBuffer()
{
    front_ = 0;
    rear_ = 0;
}