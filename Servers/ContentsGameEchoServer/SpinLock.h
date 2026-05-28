#pragma once
#include <Windows.h>

class SpinLock
{
private:
    volatile LONG lockFlag_;

public:
    SpinLock()
        : lockFlag_(0)
    {
    }

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
};