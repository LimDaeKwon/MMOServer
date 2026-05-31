#include "RingBuffer.h"

#include <Windows.h>
#include <cstdio>
#include <cstring>

RingBuffer::RingBuffer()
    : ringBuffer_(nullptr),
    front_(0),
    rear_(0),
    size_(DKServerCore::RingBufferDefaultBufferSize)
{
    ringBuffer_ = new char[DKServerCore::RingBufferDefaultBufferSize];
}

RingBuffer::RingBuffer(int bufferSize)
    : ringBuffer_(nullptr),
    front_(0),
    rear_(0),
    size_(bufferSize)
{
    ringBuffer_ = new char[bufferSize];
}

RingBuffer::~RingBuffer()
{
    delete[] ringBuffer_;
}

void RingBuffer::Resize(int newSize)
{
    if (GetUseSize() > newSize)
    {
        return;
    }

    char* temp = new char[newSize];
    int useSize = GetUseSize();

    if (Dequeue(temp, useSize) != useSize)
    {
        int error = GetLastError();

        wprintf(L"Dequeue Error %d \n", error);
        DebugBreak();
    }

    delete[] ringBuffer_;

    ringBuffer_ = temp;
    front_ = 0;
    rear_ = useSize;
    size_ = newSize;
}

int RingBuffer::GetBufferSize() const
{
    return size_;
}

int RingBuffer::GetUseSize() const
{
    int localFront = front_;
    int localRear = rear_;

    if (localFront > localRear)
    {
        return size_ - localFront + localRear;
    }

    return localRear - localFront;
}

int RingBuffer::GetFreeSize() const
{
    return size_ - GetUseSize() - 1;
}

int RingBuffer::Enqueue(const char* data, int enqueueSize)
{
    if (GetFreeSize() < enqueueSize)
    {
        return 0;
    }

    if (size_ - rear_ < enqueueSize)
    {
        int firstEnqueueSize = size_ - rear_;

        if (memcpy_s(ringBuffer_ + rear_, firstEnqueueSize, data, firstEnqueueSize) != 0)
        {
            int error = GetLastError();

            wprintf(L"memcpy_s Error %d \n", error);
            DebugBreak();
        }

        int secondEnqueueSize = enqueueSize - firstEnqueueSize;

        if (memcpy_s(ringBuffer_, secondEnqueueSize, data + firstEnqueueSize, secondEnqueueSize) != 0)
        {
            int error = GetLastError();

            wprintf(L"memcpy_s Error %d \n", error);
            DebugBreak();
        }

        rear_ = secondEnqueueSize;

        return enqueueSize;
    }

    if (memcpy_s(ringBuffer_ + rear_, enqueueSize, data, enqueueSize) != 0)
    {
        int error = GetLastError();

        wprintf(L"memcpy_s Error %d \n", error);
        DebugBreak();
    }

    if (rear_ + enqueueSize == GetBufferSize())
    {
        rear_ = 0;
    }
    else
    {
        rear_ += enqueueSize;
    }

    return enqueueSize;
}

int RingBuffer::Dequeue(char* destination, int dequeueSize)
{
    if (GetUseSize() < dequeueSize)
    {
        return 0;
    }

    if (size_ - front_ < dequeueSize)
    {
        int firstDequeueSize = size_ - front_;

        if (memcpy_s(destination, firstDequeueSize, ringBuffer_ + front_, firstDequeueSize) != 0)
        {
            int error = GetLastError();

            wprintf(L"memcpy_s Error %d \n", error);
            DebugBreak();
        }

        int secondDequeueSize = dequeueSize - firstDequeueSize;

        if (memcpy_s(destination + firstDequeueSize, secondDequeueSize, ringBuffer_, secondDequeueSize) != 0)
        {
            int error = GetLastError();

            wprintf(L"memcpy_s Error %d \n", error);
            DebugBreak();
        }

        front_ = secondDequeueSize;

        return dequeueSize;
    }

    if (memcpy_s(destination, dequeueSize, ringBuffer_ + front_, dequeueSize) != 0)
    {
        int error = GetLastError();

        wprintf(L"memcpy_s Error %d \n", error);
        DebugBreak();
    }

    if (front_ + dequeueSize == GetBufferSize())
    {
        front_ = 0;
    }
    else
    {
        front_ += dequeueSize;
    }

    return dequeueSize;
}

int RingBuffer::Peek(char* destination, int peekSize)
{
    if (GetUseSize() < peekSize)
    {
        return 0;
    }

    if (size_ - front_ < peekSize)
    {
        int firstPeekSize = size_ - front_;

        if (memcpy_s(destination, firstPeekSize, ringBuffer_ + front_, firstPeekSize) != 0)
        {
            int error = GetLastError();

            wprintf(L"memcpy_s Error %d \n", error);
            DebugBreak();
        }

        int secondPeekSize = peekSize - firstPeekSize;

        if (memcpy_s(destination + firstPeekSize, secondPeekSize, ringBuffer_, secondPeekSize) != 0)
        {
            int error = GetLastError();

            wprintf(L"memcpy_s Error %d \n", error);
            DebugBreak();
        }

        return peekSize;
    }

    if (memcpy_s(destination, peekSize, ringBuffer_ + front_, peekSize) != 0)
    {
        int error = GetLastError();

        wprintf(L"memcpy_s Error %d \n", error);
        DebugBreak();
    }

    return peekSize;
}

void RingBuffer::ClearBuffer()
{
    front_ = 0;
    rear_ = 0;
}

int RingBuffer::DirectEnqueueSize() const
{
    int localFront = front_;
    int localRear = rear_;

    if (localRear >= localFront)
    {
        if (localFront == 0)
        {
            return size_ - localRear - 1;
        }

        return size_ - localRear;
    }

    return GetFreeSize();
}

int RingBuffer::DirectDequeueSize() const
{
    int localFront = front_;
    int localRear = rear_;
    int useSize = 0;

    if (localFront > localRear)
    {
        useSize = size_ - localFront + localRear;

        if (useSize <= size_ - localFront)
        {
            return useSize;
        }

        return size_ - localFront;
    }

    return localRear - localFront;
}

int RingBuffer::MoveRear(int moveSize)
{
    if (moveSize <= 0)
    {
        return 0;
    }

    if (size_ - rear_ < moveSize)
    {
        int firstEnqueueSize = size_ - rear_;
        int secondEnqueueSize = moveSize - firstEnqueueSize;

        rear_ = secondEnqueueSize;

        return moveSize;
    }

    if (rear_ + moveSize == size_)
    {
        rear_ = 0;
    }
    else
    {
        rear_ += moveSize;
    }

    return moveSize;
}

int RingBuffer::MoveFront(int moveSize)
{
    if (moveSize <= 0)
    {
        return 0;
    }

    if (size_ - front_ < moveSize)
    {
        int firstDequeueSize = size_ - front_;
        int secondDequeueSize = moveSize - firstDequeueSize;

        front_ = secondDequeueSize;

        return moveSize;
    }

    if (front_ + moveSize == size_)
    {
        front_ = 0;
    }
    else
    {
        front_ += moveSize;
    }

    return moveSize;
}

char* RingBuffer::GetFrontBufferPtr()
{
    return ringBuffer_ + front_;
}

char* RingBuffer::GetRearBufferPtr()
{
    return ringBuffer_ + rear_;
}

char* RingBuffer::GetStartBufferPtr()
{
    return ringBuffer_;
}