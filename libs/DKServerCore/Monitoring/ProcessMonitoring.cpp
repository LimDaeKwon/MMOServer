#include "ProcessMonitoring.h"

#include <process.h>
#include <cwchar>

ProcessMonitoring::ProcessMonitoring()
    : processUserMemoryQueryString_(),
    processUserMemoryQuery_(nullptr),
    processUserMemoryTotal_(nullptr),
    processUserMemoryCounterValue_(),
    processNonPagedMemoryQueryString_(),
    processNonPagedMemoryQuery_(nullptr),
    processNonPagedMemoryTotal_(nullptr),
    processNonPagedMemoryCounterValue_(),
    updateThreadHandle_(nullptr),
    cpuUsage_()
{
    updateThreadHandle_ = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, UpdateThread, this, 0, nullptr));

    wchar_t path[MAX_PATH] = { 0 };
    GetModuleFileNameW(nullptr, path, MAX_PATH);

    wchar_t* exeName = wcsrchr(path, L'\\');

    if (exeName == nullptr)
    {
        exeName = path;
    }
    else
    {
        ++exeName;
    }

    wchar_t* movePointer = exeName;

    while (*movePointer != L'\0')
    {
        if (*movePointer == L'.')
        {
            *movePointer = L'\0';
            break;
        }

        ++movePointer;
    }

    wsprintf(processUserMemoryQueryString_, L"\\Process(%s)\\Private Bytes", exeName);

    PdhOpenQuery(nullptr, 0, &processUserMemoryQuery_);
    PdhAddCounter(processUserMemoryQuery_, processUserMemoryQueryString_, 0, &processUserMemoryTotal_);
    PdhCollectQueryData(processUserMemoryQuery_);

    wsprintf(processNonPagedMemoryQueryString_, L"\\Process(%s)\\Pool Nonpaged Bytes", exeName);

    PdhOpenQuery(nullptr, 0, &processNonPagedMemoryQuery_);
    PdhAddCounter(processNonPagedMemoryQuery_, processNonPagedMemoryQueryString_, 0, &processNonPagedMemoryTotal_);
    PdhCollectQueryData(processNonPagedMemoryQuery_);
}

ProcessMonitoring::~ProcessMonitoring()
{
}

double ProcessMonitoring::GetProcessUserMemory() const
{
    return processUserMemoryCounterValue_.doubleValue;
}

int ProcessMonitoring::GetProcessUserMemoryMBytes() const
{
    LONGLONG userBytes = static_cast<LONGLONG>(processUserMemoryCounterValue_.doubleValue);
    int userMBytes = static_cast<int>(userBytes / (1024LL * 1024LL));

    return userMBytes;
}

double ProcessMonitoring::GetProcessNonPagedMemory() const
{
    return processNonPagedMemoryCounterValue_.doubleValue;
}

unsigned int WINAPI ProcessMonitoring::UpdateThread(void* thisPointer)
{
    ProcessMonitoring* monitor = static_cast<ProcessMonitoring*>(thisPointer);

    while (true)
    {
        Sleep(100);

        monitor->UpdateCpuTime();

        PdhCollectQueryData(monitor->processUserMemoryQuery_);
        PdhGetFormattedCounterValue(monitor->processUserMemoryTotal_, PDH_FMT_DOUBLE, nullptr, &monitor->processUserMemoryCounterValue_);

        PdhCollectQueryData(monitor->processNonPagedMemoryQuery_);
        PdhGetFormattedCounterValue(monitor->processNonPagedMemoryTotal_, PDH_FMT_DOUBLE, nullptr, &monitor->processNonPagedMemoryCounterValue_);
    }

    return 0;
}

void ProcessMonitoring::UpdateCpuTime()
{
    cpuUsage_.UpdateCpuTime();
}

float ProcessMonitoring::ProcessTotal() const
{
    return cpuUsage_.ProcessTotal();
}

float ProcessMonitoring::ProcessUser() const
{
    return cpuUsage_.ProcessUser();
}

float ProcessMonitoring::ProcessKernel() const
{
    return cpuUsage_.ProcessKernel();
}