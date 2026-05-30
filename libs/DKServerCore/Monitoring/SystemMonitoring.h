#pragma once

#include <Windows.h>
#include <Pdh.h>

#include "CpuUsage.h"

constexpr int NetworkInterfaceCount = 3;

class SystemMonitoring
{
public:
    SystemMonitoring();
    virtual ~SystemMonitoring();

    double GetServerNonPagedBytes() const;
    int GetServerNonPagedMBytes() const;
    double GetServerAvailableMBytes() const;

    double GetServerNetSendKBytes() const;
    double GetServerNetRecvKBytes() const;

    void UpdateCpuTime();

    float ProcessorTotal() const;
    float ProcessorUser() const;
    float ProcessorKernel() const;

    float ProcessTotal() const;
    float ProcessUser() const;
    float ProcessKernel() const;

private:
    static unsigned int WINAPI UpdateThread(void* thisPointer);

private:
    PDH_HQUERY serverNonPagedQuery_;
    PDH_HCOUNTER serverNonPagedTotal_;
    PDH_FMT_COUNTERVALUE serverNonPagedCounterValue_;

    PDH_HQUERY serverAvailableMemoryQuery_;
    PDH_HCOUNTER serverAvailableMemoryTotal_;
    PDH_FMT_COUNTERVALUE serverAvailableMemoryCounterValue_;

    PDH_HQUERY serverNetSendQuery_[NetworkInterfaceCount];
    PDH_HCOUNTER serverNetSendTotal_[NetworkInterfaceCount];
    PDH_FMT_COUNTERVALUE serverNetSendCounterValue_[NetworkInterfaceCount];

    PDH_HQUERY serverNetRecvQuery_[NetworkInterfaceCount];
    PDH_HCOUNTER serverNetRecvTotal_[NetworkInterfaceCount];
    PDH_FMT_COUNTERVALUE serverNetRecvCounterValue_[NetworkInterfaceCount];

    HANDLE updateThreadHandle_;

    CpuUsage cpuUsage_;
};