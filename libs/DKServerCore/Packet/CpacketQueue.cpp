#include "CPacketQueue.h"

#include "CPacket.h"

CPacketQueue::CPacketQueue()
    : packetQueue_(nullptr),
    front_(0),
    rear_(0),
    size_(DKServerCore::PacketQueueDefaultBufferSize)
{
    packetQueue_ = new CPacket * [DKServerCore::PacketQueueDefaultBufferSize];
}

CPacketQueue::CPacketQueue(int bufferSize)
    : packetQueue_(nullptr),
    front_(0),
    rear_(0),
    size_(bufferSize)
{
    packetQueue_ = new CPacket * [bufferSize];
}

CPacketQueue::~CPacketQueue()
{
    delete[] packetQueue_;
}

void CPacketQueue::Resize(int newSize)
{
}

int CPacketQueue::GetBufferSize() const
{
    return size_;
}

int CPacketQueue::GetUseSize() const
{
    int localFront = front_;
    int localRear = rear_;

    if (localFront > localRear)
    {
        return size_ - localFront + localRear;
    }

    return localRear - localFront;
}

int CPacketQueue::GetFreeSize() const
{
    return size_ - GetUseSize() - 1;
}

int CPacketQueue::IsEmpty() const
{
    return rear_ == front_;
}

bool CPacketQueue::Enqueue(CPacket* data)
{
    if (GetFreeSize() == 0)
    {
        return false;
    }

    packetQueue_[rear_] = data;

    if (rear_ == size_ - 1)
    {
        rear_ = 0;
    }
    else
    {
        ++rear_;
    }

    return true;
}

bool CPacketQueue::Dequeue(CPacket** out)
{
    if (out == nullptr)
    {
        return false;
    }

    if (IsEmpty())
    {
        return false;
    }

    *out = packetQueue_[front_];

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


void CPacketQueue::ClearBuffer()
{
    front_ = 0;
    rear_ = 0;
}