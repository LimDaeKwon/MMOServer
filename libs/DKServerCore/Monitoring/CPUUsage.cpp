#include "CpuUsage.h"

#include <TlHelp32.h>
#include <cwchar>

CpuUsage::CpuUsage(HANDLE processHandle)
    : processHandle_(processHandle),
    numberOfProcessors_(0),
    processorTotal_(0.0f),
    processorUser_(0.0f),
    processorKernel_(0.0f),
    processTotal_(0.0f),
    processUser_(0.0f),
    processKernel_(0.0f)
{
    if (processHandle_ == INVALID_HANDLE_VALUE)
    {
        processHandle_ = GetCurrentProcess();
    }

    SYSTEM_INFO systemInfo;
    GetSystemInfo(&systemInfo);

    numberOfProcessors_ = static_cast<int>(systemInfo.dwNumberOfProcessors);

    processorLastKernel_.QuadPart = 0;
    processorLastUser_.QuadPart = 0;
    processorLastIdle_.QuadPart = 0;

    processLastKernel_.QuadPart = 0;
    processLastUser_.QuadPart = 0;
    processLastTime_.QuadPart = 0;

    UpdateCpuTime();
}

HANDLE CpuUsage::GetProcessHandle(const wchar_t* processName)
{
    PROCESSENTRY32 processEntry;
    processEntry.dwSize = sizeof(PROCESSENTRY32);

    DWORD processId = 0;

    HANDLE snapshotHandle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (snapshotHandle == INVALID_HANDLE_VALUE)
    {
        return nullptr;
    }

    if (Process32First(snapshotHandle, &processEntry))
    {
        do
        {
            if (wcscmp(processEntry.szExeFile, processName) == 0)
            {
                processId = processEntry.th32ProcessID;
                break;
            }
        } while (Process32Next(snapshotHandle, &processEntry));
    }

    CloseHandle(snapshotHandle);

    if (processId == 0)
    {
        return nullptr;
    }

    HANDLE processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);

    if (processHandle == nullptr)
    {
        return nullptr;
    }

    return processHandle;
}

void CpuUsage::UpdateCpuTime()
{
    ULARGE_INTEGER idle;
    ULARGE_INTEGER kernel;
    ULARGE_INTEGER user;

    if (GetSystemTimes(
        reinterpret_cast<PFILETIME>(&idle),
        reinterpret_cast<PFILETIME>(&kernel),
        reinterpret_cast<PFILETIME>(&user)) == false)
    {
        return;
    }

    ULONGLONG kernelDiff = kernel.QuadPart - processorLastKernel_.QuadPart;
    ULONGLONG userDiff = user.QuadPart - processorLastUser_.QuadPart;
    ULONGLONG idleDiff = idle.QuadPart - processorLastIdle_.QuadPart;
    ULONGLONG total = kernelDiff + userDiff;

    if (total == 0)
    {
        processorUser_ = 0.0f;
        processorKernel_ = 0.0f;
        processorTotal_ = 0.0f;
    }
    else
    {
        processorTotal_ = static_cast<float>(static_cast<double>(total - idleDiff) / static_cast<double>(total) * 100.0);

        processorUser_ = static_cast<float>(static_cast<double>(userDiff) / static_cast<double>(total) * 100.0);

        processorKernel_ = static_cast<float>(static_cast<double>(kernelDiff - idleDiff) / static_cast<double>(total) * 100.0);
    }

    processorLastKernel_ = kernel;
    processorLastUser_ = user;
    processorLastIdle_ = idle;

    ULARGE_INTEGER none;
    ULARGE_INTEGER nowTime;

    GetSystemTimeAsFileTime(reinterpret_cast<LPFILETIME>(&nowTime));

    if (GetProcessTimes(
        processHandle_,
        reinterpret_cast<LPFILETIME>(&none),
        reinterpret_cast<LPFILETIME>(&none),
        reinterpret_cast<LPFILETIME>(&kernel),
        reinterpret_cast<LPFILETIME>(&user)) == false)
    {
        return;
    }

    ULONGLONG timeDiff = nowTime.QuadPart - processLastTime_.QuadPart;
    userDiff = user.QuadPart - processLastUser_.QuadPart;
    kernelDiff = kernel.QuadPart - processLastKernel_.QuadPart;
    total = kernelDiff + userDiff;

    if (timeDiff == 0 || numberOfProcessors_ == 0)
    {
        processTotal_ = 0.0f;
        processKernel_ = 0.0f;
        processUser_ = 0.0f;
    }
    else
    {
        processTotal_ = static_cast<float>(static_cast<double>(total) / static_cast<double>(numberOfProcessors_) / static_cast<double>(timeDiff) * 100.0);

        processKernel_ = static_cast<float>(static_cast<double>(kernelDiff) / static_cast<double>(numberOfProcessors_) / static_cast<double>(timeDiff) * 100.0);

        processUser_ = static_cast<float>(static_cast<double>(userDiff) / static_cast<double>(numberOfProcessors_) / static_cast<double>(timeDiff) * 100.0);
    }

    processLastTime_ = nowTime;
    processLastKernel_ = kernel;
    processLastUser_ = user;
}

float CpuUsage::ProcessorTotal() const
{
    return processorTotal_;
}

float CpuUsage::ProcessorUser() const
{
    return processorUser_;
}

float CpuUsage::ProcessorKernel() const
{
    return processorKernel_;
}

float CpuUsage::ProcessTotal() const
{
    return processTotal_;
}

float CpuUsage::ProcessUser() const
{
    return processUser_;
}

float CpuUsage::ProcessKernel() const
{
    return processKernel_;
}