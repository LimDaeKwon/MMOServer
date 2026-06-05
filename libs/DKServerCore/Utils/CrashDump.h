#pragma once

#include <Windows.h>
#include <Psapi.h>
#include <DbgHelp.h>
#include <crtdbg.h>
#include <cstdint>
#include <cstdio>

#pragma comment(lib, "Dbghelp.lib")

class CrashDump
{
public:
    CrashDump()
    {
        _set_invalid_parameter_handler(MyInvalidParameterHandler);

        _CrtSetReportMode(_CRT_WARN, 0);
        _CrtSetReportMode(_CRT_ASSERT, 0);
        _CrtSetReportMode(_CRT_ERROR, 0);

        _CrtSetReportHook(CustomReportHook);

        _set_purecall_handler(MyPurecallHandler);

        SetHandlerDump();
    }

    static void Crash()
    {
        int* pointer = nullptr;

        *pointer = 0;
    }

    static void SetHandlerDump()
    {
        SetUnhandledExceptionFilter(MyExceptionFilter);
    }

private:
    static LONG WINAPI MyExceptionFilter(PEXCEPTION_POINTERS exceptionPointers)
    {
        int workingMemory = 0;
        SYSTEMTIME time;

        long localDumpCount = InterlockedIncrement(&DumpCount());

        HANDLE process = GetCurrentProcess();
        PROCESS_MEMORY_COUNTERS processMemoryCounters;

        if (GetProcessMemoryInfo(
            process,
            &processMemoryCounters,
            sizeof(processMemoryCounters)))
        {
            workingMemory = static_cast<int>(processMemoryCounters.WorkingSetSize / 1024 / 1024);
        }

        WCHAR fileName[MAX_PATH];

        GetLocalTime(&time);

        swprintf_s(fileName, MAX_PATH, L"Dump_%d%02d%02d_%02d.%02d.%02d_%d_%dMB.dmp", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond, localDumpCount, workingMemory);

        wprintf(L"\n\n\n!!! Crash Error ..... %d.%d.%d / %d:%d:%d \n", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond);

        HANDLE dumpFile = CreateFile(fileName, GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

        if (dumpFile != INVALID_HANDLE_VALUE)
        {
            MINIDUMP_EXCEPTION_INFORMATION minidumpExceptionInformation;

            minidumpExceptionInformation.ThreadId = GetCurrentThreadId();
            minidumpExceptionInformation.ExceptionPointers = exceptionPointers;
            minidumpExceptionInformation.ClientPointers = TRUE;

            BOOL result = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), dumpFile, MiniDumpWithFullMemory, &minidumpExceptionInformation, nullptr, nullptr);

            if (result == FALSE)
            {
                DWORD error = GetLastError();

                wprintf(L"MiniDumpWriteDump Error %lu\n", error);
            }

            CloseHandle(dumpFile);

            wprintf(L"Save Finish");
        }

        return EXCEPTION_EXECUTE_HANDLER;
    }

    static void MyInvalidParameterHandler(const wchar_t* expression, const wchar_t* function, const wchar_t* file, unsigned int line, uintptr_t reserved)
    {
        UNREFERENCED_PARAMETER(expression);
        UNREFERENCED_PARAMETER(function);
        UNREFERENCED_PARAMETER(file);
        UNREFERENCED_PARAMETER(line);
        UNREFERENCED_PARAMETER(reserved);

        Crash();
    }

    static int CustomReportHook(int reportType, char* message, int* returnValue)
    {
        UNREFERENCED_PARAMETER(reportType);
        UNREFERENCED_PARAMETER(message);
        UNREFERENCED_PARAMETER(returnValue);

        Crash();

        return TRUE;
    }

    static void MyPurecallHandler()
    {
        Crash();
    }

    static volatile long& DumpCount()
    {
        static volatile long dumpCount = 0;

        return dumpCount;
    }
};