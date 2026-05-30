#pragma once

#include <Windows.h>

class CpuUsage
{
public:
    CpuUsage(HANDLE processHandle = INVALID_HANDLE_VALUE);

    static HANDLE GetProcessHandle(const wchar_t* processName);

    void UpdateCpuTime();

    float ProcessorTotal() const;
    float ProcessorUser() const;
    float ProcessorKernel() const;

    float ProcessTotal() const;
    float ProcessUser() const;
    float ProcessKernel() const;

private:
    HANDLE processHandle_;
    int numberOfProcessors_;

    float processorTotal_;
    float processorUser_;
    float processorKernel_;

    float processTotal_;
    float processUser_;
    float processKernel_;

    ULARGE_INTEGER processorLastKernel_;
    ULARGE_INTEGER processorLastUser_;
    ULARGE_INTEGER processorLastIdle_;

    ULARGE_INTEGER processLastKernel_;
    ULARGE_INTEGER processLastUser_;
    ULARGE_INTEGER processLastTime_;
};