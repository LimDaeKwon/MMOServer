#include "SystemMonitoring.h"

#include <process.h>

SystemMonitoring::SystemMonitoring()
    : serverNonPagedQuery_(nullptr),
    serverNonPagedTotal_(nullptr),
    serverNonPagedCounterValue_(),
    serverAvailableMemoryQuery_(nullptr),
    serverAvailableMemoryTotal_(nullptr),
    serverAvailableMemoryCounterValue_(),
    serverNetSendQuery_(),
    serverNetSendTotal_(),
    serverNetSendCounterValue_(),
    serverNetRecvQuery_(),
    serverNetRecvTotal_(),
    serverNetRecvCounterValue_(),
    updateThreadHandle_(nullptr),
    cpuUsage_()
{
    updateThreadHandle_ = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, UpdateThread, this, 0, nullptr));

    const wchar_t* interfaceNames[NetworkInterfaceCount] =
    {
        L"Intel[R] Ethernet Controller X540-AT2 _2",
        L"Intel[R] I210 Gigabit Network Connection",
        L"Intel[R] I210 Gigabit Network Connection _2"
    };

    PdhOpenQuery(nullptr, 0, &serverNonPagedQuery_);
    PdhAddCounter(serverNonPagedQuery_, L"\\Memory\\Pool Nonpaged Bytes", 0, &serverNonPagedTotal_);
    PdhCollectQueryData(serverNonPagedQuery_);

    PdhOpenQuery(nullptr, 0, &serverAvailableMemoryQuery_);
    PdhAddCounter(serverAvailableMemoryQuery_, L"\\Memory\\Available MBytes", 0, &serverAvailableMemoryTotal_);
    PdhCollectQueryData(serverAvailableMemoryQuery_);

    wchar_t sendPath[256];
    wchar_t recvPath[256];

    for (int i = 0; i < NetworkInterfaceCount; ++i)
    {
        wsprintf(sendPath, L"\\Network Interface(%s)\\Bytes Sent/sec", interfaceNames[i]);

        wsprintf(recvPath, L"\\Network Interface(%s)\\Bytes Received/sec", interfaceNames[i]);

        PdhOpenQuery(nullptr, 0, &serverNetSendQuery_[i]);
        PdhAddCounter(serverNetSendQuery_[i], sendPath, 0, &serverNetSendTotal_[i]);
        PdhCollectQueryData(serverNetSendQuery_[i]);

        PdhOpenQuery(nullptr, 0, &serverNetRecvQuery_[i]);
        PdhAddCounter(serverNetRecvQuery_[i], recvPath, 0, &serverNetRecvTotal_[i]);
        PdhCollectQueryData(serverNetRecvQuery_[i]);
    }
}

SystemMonitoring::~SystemMonitoring()
{
}

double SystemMonitoring::GetServerNonPagedBytes() const
{
    return serverNonPagedCounterValue_.doubleValue;
}

int SystemMonitoring::GetServerNonPagedMBytes() const
{
    LONGLONG nonPagedBytes = static_cast<LONGLONG>(serverNonPagedCounterValue_.doubleValue);
    int nonPagedMBytes = static_cast<int>(nonPagedBytes / (1024LL * 1024LL));

    return nonPagedMBytes;
}

double SystemMonitoring::GetServerAvailableMBytes() const
{
    return serverAvailableMemoryCounterValue_.doubleValue;
}

double SystemMonitoring::GetServerNetSendKBytes() const
{
    double totalSend = 0.0;

    for (int i = 0; i < NetworkInterfaceCount; ++i)
    {
        totalSend += serverNetSendCounterValue_[i].doubleValue;
    }

    return totalSend / 1024.0;
}

double SystemMonitoring::GetServerNetRecvKBytes() const
{
    double totalRecv = 0.0;

    for (int i = 0; i < NetworkInterfaceCount; ++i)
    {
        totalRecv += serverNetRecvCounterValue_[i].doubleValue;
    }

    return totalRecv / 1024.0;
}

unsigned int WINAPI SystemMonitoring::UpdateThread(void* thisPointer)
{
    SystemMonitoring* monitor = static_cast<SystemMonitoring*>(thisPointer);

    while (true)
    {
        Sleep(1000);

        monitor->UpdateCpuTime();

        PdhCollectQueryData(monitor->serverNonPagedQuery_);
        PdhGetFormattedCounterValue(monitor->serverNonPagedTotal_, PDH_FMT_DOUBLE, nullptr, &monitor->serverNonPagedCounterValue_);

        PdhCollectQueryData(monitor->serverAvailableMemoryQuery_);
        PdhGetFormattedCounterValue(monitor->serverAvailableMemoryTotal_, PDH_FMT_DOUBLE, nullptr, &monitor->serverAvailableMemoryCounterValue_);

        for (int i = 0; i < NetworkInterfaceCount; ++i)
        {
            PdhCollectQueryData(monitor->serverNetSendQuery_[i]);
            PdhGetFormattedCounterValue(monitor->serverNetSendTotal_[i], PDH_FMT_DOUBLE, nullptr, &monitor->serverNetSendCounterValue_[i]);

            PdhCollectQueryData(monitor->serverNetRecvQuery_[i]);
            PdhGetFormattedCounterValue(monitor->serverNetRecvTotal_[i], PDH_FMT_DOUBLE, nullptr, &monitor->serverNetRecvCounterValue_[i]);
        }
    }

    return 0;
}

void SystemMonitoring::UpdateCpuTime()
{
    cpuUsage_.UpdateCpuTime();
}

float SystemMonitoring::ProcessorTotal() const
{
    return cpuUsage_.ProcessorTotal();
}

float SystemMonitoring::ProcessorUser() const
{
    return cpuUsage_.ProcessorUser();
}

float SystemMonitoring::ProcessorKernel() const
{
    return cpuUsage_.ProcessorKernel();
}

float SystemMonitoring::ProcessTotal() const
{
    return cpuUsage_.ProcessTotal();
}

float SystemMonitoring::ProcessUser() const
{
    return cpuUsage_.ProcessUser();
}

float SystemMonitoring::ProcessKernel() const
{
    return cpuUsage_.ProcessKernel();
}