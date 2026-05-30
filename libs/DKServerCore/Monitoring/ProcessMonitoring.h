#pragma once

#include <Windows.h>
#include <Pdh.h>

#include "CpuUsage.h"

#pragma comment(lib, "Pdh.lib")

class ProcessMonitoring
{
public:
    ProcessMonitoring();
    virtual ~ProcessMonitoring();

    double GetProcessUserMemory() const;
    int GetProcessUserMemoryMBytes() const;

    double GetProcessNonPagedMemory() const;

    void UpdateCpuTime();

    float ProcessTotal() const;
    float ProcessUser() const;
    float ProcessKernel() const;

private:
    static unsigned int WINAPI UpdateThread(void* thisPointer);

private:
    WCHAR processUserMemoryQueryString_[200];
    PDH_HQUERY processUserMemoryQuery_;
    PDH_HCOUNTER processUserMemoryTotal_;
    PDH_FMT_COUNTERVALUE processUserMemoryCounterValue_;

    WCHAR processNonPagedMemoryQueryString_[200];
    PDH_HQUERY processNonPagedMemoryQuery_;
    PDH_HCOUNTER processNonPagedMemoryTotal_;
    PDH_FMT_COUNTERVALUE processNonPagedMemoryCounterValue_;

    HANDLE updateThreadHandle_;

    CpuUsage cpuUsage_;
};