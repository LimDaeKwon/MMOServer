#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <crtdbg.h>
#include <Windows.h>
#include <psapi.h>
#include <minidumpapiset.h>

#pragma comment (lib,"Dbghelp.lib")

class CrashDump
{
public:

	CrashDump()
	{
		//dump_count = 0;


		_invalid_parameter_handler old_handler;
		_invalid_parameter_handler new_handler;

		new_handler = MyInvalidParameterHandler;
		old_handler = _set_invalid_parameter_handler(new_handler);

		_CrtSetReportMode(_CRT_WARN, 0);
		_CrtSetReportMode(_CRT_ASSERT, 0);
		_CrtSetReportMode(_CRT_ERROR, 0);

		_CrtSetReportHook(CustomReportHook);

		_set_purecall_handler(MyPurecallHandler);

		SetHandlerDump();

	}

	static void Crash()
	{
		int* p = nullptr;
		*p = 0;

	}

	static LONG WINAPI MyExcpetionFilter(__in PEXCEPTION_POINTERS exception_pointer)
	{
		int working_memory = 0;
		SYSTEMTIME time;

		long local_dump_count = InterlockedIncrement(&dump_count);

		HANDLE process = 0;
		PROCESS_MEMORY_COUNTERS pmc;

		process = GetCurrentProcess();

		if (NULL == process)
		{
			return 0;
		}

		if (GetProcessMemoryInfo(process, &pmc, sizeof(pmc)))
		{
			working_memory = (int)(pmc.WorkingSetSize / 1024 / 1024);
		}
		CloseHandle(process);

		WCHAR filename[MAX_PATH];

		GetLocalTime(&time);
		wsprintf(filename, L"Dump_%d%02d%02d_%02d.%02d.%02d_%d_%dMB.dmp",
			time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond, local_dump_count, working_memory);

		wprintf(L"\n\n\n!!! Crash Error ..... %d.%d.%d / %d:%d:%d \n", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond);

		HANDLE dump_file = CreateFile(filename, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

		if (dump_file != INVALID_HANDLE_VALUE)
		{
			_MINIDUMP_EXCEPTION_INFORMATION minidump_exception_information;

			minidump_exception_information.ThreadId = GetCurrentThreadId();
			minidump_exception_information.ExceptionPointers = exception_pointer;
			minidump_exception_information.ClientPointers = TRUE;

			MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), dump_file, MiniDumpWithFullMemory, &minidump_exception_information, NULL, NULL);

			CloseHandle(dump_file);

			wprintf(L"Save Finish");

		}

		return EXCEPTION_EXECUTE_HANDLER;
	}



	static void SetHandlerDump()
	{
		SetUnhandledExceptionFilter(MyExcpetionFilter);
	}

	static void MyInvalidParameterHandler(const wchar_t* expression, const wchar_t* function, const wchar_t* file, unsigned int line, uintptr_t reserved)
	{
		Crash();
	}

	static int CustomReportHook(int ireposttype, char* message, int* returnvalue)
	{
		Crash();
		return true;
	}

	static void MyPurecallHandler()
	{
		Crash();
	}

	static long dump_count ;
};


long CrashDump::dump_count = 0;