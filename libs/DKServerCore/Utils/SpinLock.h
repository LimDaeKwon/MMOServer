#pragma once

#include <Windows.h>

class SpinLock
{
public:
    SpinLock()
        : lockFlag_(0)
    {
    }

    SpinLock(const SpinLock&) = delete;
    SpinLock& operator=(const SpinLock&) = delete;

    void Lock()
    {
        while (InterlockedExchange(&lockFlag_, 1) == 1)
        {
            while (lockFlag_ == 1)
            {
                YieldProcessor();
            }
        }
    }

    void Unlock()
    {
        InterlockedExchange(&lockFlag_, 0);
    }

private:
    volatile LONG lockFlag_;
};

class SpinLockGuard
{
public:
    SpinLockGuard(SpinLock& spinLock)
        : spinLock_(spinLock)
    {
        spinLock_.Lock();
    }

    ~SpinLockGuard()
    {
        spinLock_.Unlock();
    }

    SpinLockGuard(const SpinLockGuard&) = delete;
    SpinLockGuard& operator=(const SpinLockGuard&) = delete;

private:
    SpinLock& spinLock_;
};